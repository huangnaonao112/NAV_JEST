#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
orb_file_player.py — 报文文件回放器（TCP 注入）

把 orb_day_generator.py 生成的报文文件按指定时长匀速发给仿真软件，
模拟"外部解算器实时推流"。本机充当 TCP 服务端（软件会自动重连 127.0.0.1:8000）。

用法：
  # 一天的量 120 秒播完（默认），循环 3 遍
  python orb_file_player.py --file playback/orb_day_full.txt --seconds 120 --loop 3

  # 5 分钟抽稀版 30 秒播完
  python orb_file_player.py --file playback/orb_day_5min.txt --seconds 30
"""
import argparse
import socket
import sys
import time


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--file', required=True, help='报文文件路径')
    ap.add_argument('--seconds', type=float, default=120.0,
                    help='整个文件的目标播放时长（秒）')
    ap.add_argument('--loop', type=int, default=1, help='循环次数')
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=8000)
    ap.add_argument('--wait', type=float, default=120.0, help='等待连接超时（秒）')
    args = ap.parse_args()

    with open(args.file, encoding='utf-8') as f:
        lines = [l.rstrip('\r\n') for l in f if l.strip()]
    n = len(lines)
    if n < 2:
        sys.exit('file too small: %s' % args.file)
    delay = max(0.001, args.seconds / n)
    print('[player] %s: %d lines, %.1f ms/line, loop=%d'
          % (args.file, n, delay * 1000, args.loop), flush=True)

    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((args.host, args.port))
    srv.listen(1)
    srv.settimeout(args.wait)
    print('[player] listening on %s:%d ...' % (args.host, args.port), flush=True)
    conn, addr = srv.accept()
    conn.settimeout(5.0)
    print('[player] connected', addr, flush=True)

    for lap in range(args.loop):
        t0 = time.time()
        sent = 0
        for idx, line in enumerate(lines):
            try:
                conn.sendall((line + '\n').encode('utf-8'))
            except Exception as e:
                print('[player] lap %d: send failed at %d/%d: %r'
                      % (lap, idx, n, e), flush=True)
                return
            sent += 1
            # 匀速节拍（扣除已耗时间，避免累计漂移）
            target = t0 + delay * (idx + 1)
            pause = target - time.time()
            if pause > 0:
                time.sleep(pause)
            if idx % max(1, n // 10) == 0:
                print('[player] lap %d %3d%% (%d/%d)'
                      % (lap, 100 * idx // n, idx, n), flush=True)
        print('[player] lap %d done: %d lines in %.1fs'
              % (lap, sent, time.time() - t0), flush=True)

    print('[player] finished, closing', flush=True)
    time.sleep(2.0)


if __name__ == '__main__':
    main()
