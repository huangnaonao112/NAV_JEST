#!/usr/bin/env python3
"""
TCP 收发工具 - 服务端模式
- 监听端口，等待外部客户端连接
- 发送: 读取 TXT 文件内容
- 左右两栏对半显示: 发送(左) / 接收(右)
"""

import os
import sys

# 设置 Tcl/Tk 环境变量（打包后需要）。
# 注意: PyInstaller 6 的数据目录是 _tcl_data/_tk_data（旧版为 tcl/tk），
# 写死任何一种都会在另一种布局下 Tcl 初始化失败（init.tcl 找不到），
# 因此按实际存在的目录自适应。
if getattr(sys, 'frozen', False):
    base_path = sys._MEIPASS
    exe_dir = os.path.dirname(sys.executable)
    tcl_cands = [
        os.path.join(base_path, '_tcl_data'),                       # PyInstaller 6
        os.path.join(exe_dir, '_internal', '_tcl_data'),            # PyInstaller 6 onedir
        os.path.join(base_path, 'tcl', 'tcl8.6'),                   # PyInstaller ≤5
        os.path.join(exe_dir, 'tcl', 'tcl8.6'),
    ]
    for cand in tcl_cands:
        if os.path.isfile(os.path.join(cand, 'init.tcl')):
            os.environ['TCL_LIBRARY'] = cand
            os.environ['TK_LIBRARY'] = cand.replace('_tcl_data', '_tk_data')                                            .replace('tcl8.6', 'tk8.6')
            break

import tkinter as tk
from tkinter import ttk, scrolledtext
import socket
import threading
import time


class TcpTransceiverApp:
    def __init__(self, root):
        self.root = root
        self.root.title("TCP 收发工具")
        self.root.geometry("900x600")
        self.root.resizable(True, True)

        # 服务端状态
        self.server_running = False
        self.server_socket = None
        self.server_clients = []
        self.server_thread = None

        # 默认数据文件路径
        self.txt_file_path = ""

        self.setup_ui()

    def setup_ui(self):
        # ======== 第一行：端口、连接按钮 ========
        row1 = ttk.Frame(self.root)
        row1.pack(fill=tk.X, padx=10, pady=5)

        ttk.Label(row1, text="端口:").pack(side=tk.LEFT)
        self.port_var = tk.StringVar(value="8000")
        ttk.Entry(row1, textvariable=self.port_var, width=8).pack(side=tk.LEFT, padx=(5, 20))

        self.connect_btn = ttk.Button(row1, text="连接", command=self.toggle_connect, width=10)
        self.connect_btn.pack(side=tk.LEFT, padx=5)

        self.status_label = ttk.Label(row1, text="● 未连接", foreground="gray")
        self.status_label.pack(side=tk.LEFT, padx=10)

        # ======== 第二行：路径输入、发送、暂停按钮 ========
        row2 = ttk.Frame(self.root)
        row2.pack(fill=tk.X, padx=10, pady=5)

        ttk.Label(row2, text="数据路径:").pack(side=tk.LEFT)
        self.path_var = tk.StringVar()
        self.path_entry = ttk.Entry(row2, textvariable=self.path_var, width=40)
        self.path_entry.pack(side=tk.LEFT, padx=(5, 5))

        self.browse_btn = ttk.Button(row2, text="浏览", command=self.browse_file, width=6)
        self.browse_btn.pack(side=tk.LEFT, padx=(0, 10))

        # 发送模式选择
        ttk.Label(row2, text="模式:").pack(side=tk.LEFT, padx=(10, 0))
        self.mode_var = tk.StringVar(value="ASCII码")
        self.mode_combo = ttk.Combobox(row2, textvariable=self.mode_var,
                                        values=["ASCII码", "16进制"], width=8, state="readonly")
        self.mode_combo.pack(side=tk.LEFT, padx=(5, 10))
        self.mode_combo.bind("<<ComboboxSelected>>", self.on_mode_change)

        self.send_btn = ttk.Button(row2, text="发送", command=self.send_file, width=10)
        self.send_btn.pack(side=tk.LEFT, padx=5)

        self.pause_btn = ttk.Button(row2, text="暂停", command=self.toggle_pause, width=10, state=tk.DISABLED)
        self.pause_btn.pack(side=tk.LEFT, padx=5)

        self.resend_btn = ttk.Button(row2, text="重新发送", command=self.resend_file, width=10)
        self.resend_btn.pack(side=tk.LEFT, padx=5)

        # 发送频率设置
        ttk.Label(row2, text="频率:").pack(side=tk.LEFT, padx=(20, 0))
        self.freq_var = tk.StringVar(value="2")
        ttk.Entry(row2, textvariable=self.freq_var, width=5).pack(side=tk.LEFT, padx=(5, 0))
        ttk.Label(row2, text="Hz").pack(side=tk.LEFT, padx=(2, 10))

        # 包头设置（仅16进制模式显示）
        self.header_label = ttk.Label(row2, text="包头:")
        self.header_var = tk.StringVar(value="01 AB")
        self.header_entry = ttk.Entry(row2, textvariable=self.header_var, width=10)
        # 默认隐藏，切换到16进制模式时才显示

        self.paused = False  # 发送暂停标志
        self.sending = False  # 发送中标志
        self.send_thread = None
        self.current_line_index = 0  # 当前发送到的行索引
        self.lines = []  # 文件所有行
        self.binary_data = b''  # 16进制模式下的二进制数据
        self.bytes_sent_index = 0  # 16进制模式下已发送的字节索引

        # ======== 左右两栏对半显示区 ========
        display_frame = ttk.Frame(self.root)
        display_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 配置两列等宽
        display_frame.columnconfigure(0, weight=1)
        display_frame.columnconfigure(1, weight=1)
        display_frame.rowconfigure(0, weight=1)

        # 左侧 - 发送区
        left_frame = ttk.LabelFrame(display_frame, text="发送 (TX)", padding="5")
        left_frame.grid(row=0, column=0, sticky="nsew", padx=(0, 5))

        self.send_log = scrolledtext.ScrolledText(left_frame, state=tk.DISABLED,
                                                   font=("Consolas", 9), bg="white", fg="black")
        self.send_log.pack(fill=tk.BOTH, expand=True)

        # 右侧 - 接收区
        right_frame = ttk.LabelFrame(display_frame, text="接收 (RX)", padding="5")
        right_frame.grid(row=0, column=1, sticky="nsew", padx=(5, 0))

        self.recv_log = scrolledtext.ScrolledText(right_frame, state=tk.DISABLED,
                                                   font=("Consolas", 9), bg="white", fg="black")
        self.recv_log.pack(fill=tk.BOTH, expand=True)

    def browse_file(self):
        """浏览选择数据文件"""
        from tkinter import filedialog
        filetypes = [("文本文件", "*.txt"), ("所有文件", "*.*")] if self.mode_var.get() == "ASCII码" else [("所有文件", "*.*")]
        file_path = filedialog.askopenfilename(
            title="选择数据文件",
            filetypes=filetypes
        )
        if file_path:
            self.path_var.set(file_path)
            self.txt_file_path = file_path
            self._auto_freq(file_path)

    def _auto_freq(self, file_path):
        """加载文件后自动建议发送频率：默认整个文件约 5 分钟(300 秒)发完。
        三幕演示文件(约 600 行)配 2Hz 左右，节奏舒缓看得清。可手动改。"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                n = sum(1 for l in f if l.strip())
        except Exception:
            return
        if n <= 0:
            return
        freq = max(0.5, min(50.0, round(n / 300.0, 1)))
        self.freq_var.set(str(freq))

    def on_mode_change(self, event=None):
        """模式切换时更新界面"""
        if self.mode_var.get() == "ASCII码":
            self.header_label.pack_forget()
            self.header_entry.pack_forget()
        else:
            self.header_label.pack(side=tk.LEFT, padx=(10, 0))
            self.header_entry.pack(side=tk.LEFT, padx=(5, 0))

    def toggle_connect(self):
        """启动/停止服务端"""
        if self.server_running:
            self.stop_server()
        else:
            self.start_server()

    def start_server(self):
        """启动服务端"""
        try:
            port = int(self.port_var.get())
        except ValueError:
            self.log_send("错误: 端口格式错误")
            return

        self.server_running = True
        self.server_thread = threading.Thread(target=self.run_server, args=(port,), daemon=True)
        self.server_thread.start()

        self.connect_btn.config(text="断开")
        self.status_label.config(text="● 监听中", foreground="green")
        self.log_send(f"服务端启动，监听端口 {port}")

    def stop_server(self):
        """停止服务端"""
        self.server_running = False
        self.sending = False
        self.paused = False

        # 重置按钮状态
        self.send_btn.config(text="发送")
        self.pause_btn.config(text="暂停", state=tk.DISABLED)

        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass

        for client in self.server_clients:
            try:
                client.close()
            except:
                pass
        self.server_clients.clear()

        self.connect_btn.config(text="连接")
        self.status_label.config(text="● 已断开", foreground="gray")
        self.log_send("已断开所有连接")

    def run_server(self, port):
        """服务端主循环"""
        try:
            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(('0.0.0.0', port))
            server.listen(5)
            server.settimeout(1.0)
            self.server_socket = server
        except Exception as e:
            self.root.after(0, lambda: self.log_send(f"服务端启动失败: {e}"))
            self.server_running = False
            return

        while self.server_running:
            try:
                conn, addr = server.accept()
                self.server_clients.append(conn)
                threading.Thread(target=self.recv_from_client, args=(conn,), daemon=True).start()
            except socket.timeout:
                pass
            except:
                break

    def recv_from_client(self, conn):
        """从客户端接收数据"""
        conn.settimeout(1.0)
        while self.server_running:
            try:
                data = conn.recv(4096)
                if not data:
                    break
                try:
                    text = data.decode('utf-8')
                except:
                    text = data.hex()
                self.root.after(0, lambda t=text: self.log_recv(t))
            except socket.timeout:
                continue
            except:
                break

        if conn in self.server_clients:
            self.server_clients.remove(conn)
        try:
            conn.close()
        except:
            pass

    def toggle_pause(self):
        """暂停/继续发送"""
        if not self.sending:
            return

        self.paused = not self.paused
        if self.paused:
            self.pause_btn.config(text="继续")
            self.log_send("已暂停发送")
        else:
            self.pause_btn.config(text="暂停")
            self.log_send("继续发送")

    def resend_file(self):
        """重新发送 - 从头开始"""
        # 如果正在发送，先停止
        if self.sending:
            self.sending = False
            self.paused = False
            for _ in range(20):
                if not self.send_thread or not self.send_thread.is_alive():
                    break
                time.sleep(0.05)

        # 检查连接状态
        if not self.server_running:
            self.log_send("错误: 请先启动服务端")
            return

        # 检查文件路径
        file_path = self.path_var.get().strip()
        if not file_path:
            self.log_send("错误: 请先选择数据文件")
            return

        if not os.path.exists(file_path):
            self.log_send(f"错误: 找不到文件 {file_path}")
            return

        self.txt_file_path = file_path
        self.sending = True
        self.paused = False
        self.send_btn.config(text="停止")
        self.pause_btn.config(text="暂停", state=tk.NORMAL)
        self.log_send("重新发送...")
        self.send_thread = threading.Thread(target=self.send_loop, daemon=True)
        self.send_thread.start()

    def send_file(self):
        """开始/停止发送文件内容"""
        if self.sending:
            # 停止发送
            self.sending = False
            self.paused = False
            self.send_btn.config(text="发送")
            self.pause_btn.config(text="暂停", state=tk.DISABLED)
            self.log_send("已停止发送")
            return

        # 从输入框获取文件路径
        file_path = self.path_var.get().strip()
        if not file_path:
            self.log_send("错误: 请先选择数据文件")
            return

        if not os.path.exists(file_path):
            self.log_send(f"错误: 找不到文件 {file_path}")
            return

        self.txt_file_path = file_path
        self.sending = True
        self.paused = False
        self.send_btn.config(text="停止")
        self.pause_btn.config(text="暂停", state=tk.NORMAL)
        self.send_thread = threading.Thread(target=self.send_loop, daemon=True)
        self.send_thread.start()

    def send_loop(self):
        """循环发送，根据模式选择发送方式"""
        if self.mode_var.get() == "ASCII码":
            self.send_loop_ascii()
        else:
            self.send_loop_hex()

    def send_loop_ascii(self):
        """ASCII码模式 - 按行发送"""
        # 读取文件内容
        try:
            with open(self.txt_file_path, 'r', encoding='utf-8') as f:
                self.lines = f.readlines()
        except Exception as e:
            self.root.after(0, lambda: self.log_send(f"读取文件失败: {e}"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        if not self.lines:
            self.root.after(0, lambda: self.log_send("错误: 文件内容为空"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        total_lines = len(self.lines)
        self.current_line_index = 0
        self.root.after(0, lambda: self.log_send(f"开始发送，共 {total_lines} 行"))

        while self.current_line_index < total_lines:
            if not self.sending:
                break

            if not self.server_running:
                self.root.after(0, lambda: self.log_send("服务端已停止，停止发送"))
                break

            while self.paused and self.sending:
                time.sleep(0.1)

            if not self.sending:
                break

            # 获取当前行
            line = self.lines[self.current_line_index].rstrip('\n\r')
            if line:
                data = (line + '\n').encode('utf-8')

                for client in self.server_clients[:]:
                    try:
                        client.sendall(data)
                    except:
                        self.server_clients.remove(client)
                        try:
                            client.close()
                        except:
                            pass

                line_num = self.current_line_index + 1
                self.root.after(0, lambda l=line, n=line_num: self.log_send(f"[{n}] {l}"))

            self.current_line_index += 1

            # 读取频率，等待间隔
            try:
                freq = float(self.freq_var.get())
                if freq <= 0:
                    freq = 1.0
            except ValueError:
                freq = 1.0
            interval = 1.0 / freq

            self._wait_interval(interval)

        if self.sending:
            self.root.after(0, lambda: self.log_send("发送完成"))

        self.sending = False
        self.paused = False
        self.root.after(0, lambda: self.send_btn.config(text="发送"))
        self.root.after(0, lambda: self.pause_btn.config(text="暂停", state=tk.DISABLED))

    def send_loop_hex(self):
        """16进制模式 - 按包头分包发送"""
        # 解析包头
        header_hex = self.header_var.get().strip().replace(" ", "").upper()
        if not header_hex:
            self.root.after(0, lambda: self.log_send("错误: 请输入包头"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        # 验证包头格式并转换为字节
        try:
            if len(header_hex) % 2 != 0:
                raise ValueError("包头长度必须是偶数")
            header_bytes = bytes.fromhex(header_hex)
        except ValueError as e:
            self.root.after(0, lambda: self.log_send(f"错误: 包头格式错误 - {e}"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        if len(header_bytes) < 1 or len(header_bytes) > 5:
            self.root.after(0, lambda: self.log_send("错误: 包头长度应为1-5字节"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        # 读取二进制文件
        try:
            with open(self.txt_file_path, 'rb') as f:
                self.binary_data = f.read()
        except Exception as e:
            self.root.after(0, lambda: self.log_send(f"读取文件失败: {e}"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        if not self.binary_data:
            self.root.after(0, lambda: self.log_send("错误: 文件内容为空"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        # 按包头分包
        packets = self.split_by_header(self.binary_data, header_bytes)
        if not packets:
            self.root.after(0, lambda: self.log_send("错误: 未找到匹配包头的数据包"))
            self.sending = False
            self.root.after(0, lambda: self.send_btn.config(text="发送"))
            return

        self.current_packet_index = 0
        total_packets = len(packets)
        header_str = header_hex
        self.root.after(0, lambda: self.log_send(f"包头: {header_str}，共 {total_packets} 包"))

        while self.current_packet_index < total_packets:
            if not self.sending:
                break

            if not self.server_running:
                self.root.after(0, lambda: self.log_send("服务端已停止，停止发送"))
                break

            while self.paused and self.sending:
                time.sleep(0.1)

            if not self.sending:
                break

            # 每次发送前重新读取包头（支持动态修改）
            header_hex_new = self.header_var.get().strip().replace(" ", "").upper()
            try:
                header_bytes_new = bytes.fromhex(header_hex_new)
                if header_bytes_new != header_bytes:
                    # 包头改变了，重新分包
                    header_bytes = header_bytes_new
                    packets = self.split_by_header(self.binary_data, header_bytes)
                    total_packets = len(packets)
                    if self.current_packet_index >= total_packets:
                        self.current_packet_index = 0
                    header_str = header_hex_new
                    self.root.after(0, lambda h=header_str, t=total_packets:
                                   self.log_send(f"包头更新: {h}，共 {t} 包"))
            except:
                pass

            # 获取当前包
            if self.current_packet_index >= len(packets):
                break

            data = packets[self.current_packet_index]

            for client in self.server_clients[:]:
                try:
                    client.sendall(data)
                except:
                    self.server_clients.remove(client)
                    try:
                        client.close()
                    except:
                        pass

            # 显示包信息（每个字节加空格）
            hex_str = ' '.join(data[i:i+1].hex().upper() for i in range(len(data)))
            packet_num = self.current_packet_index + 1
            self.root.after(0, lambda h=hex_str, n=packet_num, b=len(data):
                           self.log_send(f"[包{n}] {b}字节:\n{h}"))

            self.current_packet_index += 1

            # 读取频率，等待间隔
            try:
                freq = float(self.freq_var.get())
                if freq <= 0:
                    freq = 1.0
            except ValueError:
                freq = 1.0
            interval = 1.0 / freq

            self._wait_interval(interval)

        if self.sending:
            self.root.after(0, lambda: self.log_send("发送完成"))

        self.sending = False
        self.paused = False
        self.root.after(0, lambda: self.send_btn.config(text="发送"))
        self.root.after(0, lambda: self.pause_btn.config(text="暂停", state=tk.DISABLED))

    def split_by_header(self, data, header):
        """按包头分割数据"""
        packets = []
        header_len = len(header)
        i = 0

        while i < len(data):
            # 查找包头
            if data[i:i + header_len] == header:
                # 找到包头，开始记录这一包
                start = i
                i += header_len
                # 继续找下一个包头或文件结束
                while i < len(data):
                    if data[i:i + header_len] == header:
                        break
                    i += 1
                packets.append(data[start:i])
            else:
                i += 1

        return packets

    def _wait_interval(self, interval):
        """等待指定间隔，可被暂停/停止中断"""
        wait_steps = int(interval / 0.01)
        remaining = interval
        for _ in range(max(1, wait_steps)):
            if not self.sending or self.paused:
                break
            sleep_time = min(0.01, remaining)
            time.sleep(sleep_time)
            remaining -= sleep_time
            if remaining <= 0:
                break

    def log_send(self, message):
        """添加发送日志"""
        self.send_log.config(state=tk.NORMAL)
        self.send_log.insert(tk.END, f"{message}\n")
        self.send_log.see(tk.END)
        self.send_log.config(state=tk.DISABLED)

    def log_recv(self, message):
        """添加接收日志"""
        self.recv_log.config(state=tk.NORMAL)
        self.recv_log.insert(tk.END, f"{message}\n")
        self.recv_log.see(tk.END)
        self.recv_log.config(state=tk.DISABLED)


def main():
    root = tk.Tk()
    app = TcpTransceiverApp(root)
    root.mainloop()


if __name__ == '__main__':
    main()
