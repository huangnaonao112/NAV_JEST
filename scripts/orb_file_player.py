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


def build_chunks(lines):
    """把行聚合成"快照块"：块内行一次性写入，块间才是节拍间隔。

    第一幕每快照 = $DEV/$ANT/... + $ORB + $SAT + $PNT + $DSP 多行，若按行
    匀速发送，$ORB（重置时间基准 tow）与 $DSP（更新欺骗星 M）会错开数秒
    到达，且 TCP 合包让帧间隔抖动、倍率外推过冲——两者都会造成 3D 欺骗
    卫星左右闪动。以帧起始行（$ORB/$TRJ/$MNT）分块后：
      块内原子（快照内 M 与 tow 同批到达），块间间隔均匀。
    各幕时间占比仍按行数分配，与旧的按行匀速节奏一致。
    """
    frame_starts = ('$ORB', '$TRJ', '$MNT')
    chunks = []
    cur = []
    for line in lines:
        if line.startswith(frame_starts) and cur:
            chunks.append(cur)
            cur = []
        cur.append(line)
    if cur:
        chunks.append(cur)
    return chunks


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

    # 按幕（$BEG 行）分段，各幕时间 = 总时长 × 行数占比；幕内按快照块均分
    beg_idx = [i for i, l in enumerate(lines) if l.startswith('$BEG')]
    segments = []
    for s, start in enumerate(beg_idx):
        end = beg_idx[s + 1] if s + 1 < len(beg_idx) else n
        segments.append(lines[start:end])
    head = lines[:beg_idx[0]] if beg_idx and beg_idx[0] > 0 else []

    schedule = []   # (块行列表, 块时长秒)
    for seg in segments:
        chunks = build_chunks(seg)
        seg_share = len(seg) / n * args.seconds
        per = max(0.001, seg_share / max(1, len(chunks)))
        for c in chunks:
            schedule.append((c, per))
    if head:
        schedule.insert(0, (head, 0.001))

    # 绝对目标时刻（相对 lap 起点）= 各块时长逐块累加。
    # 不能写成 per*(idx+1)：三幕块时长不同，后幕的目标时刻会落后墙钟、
    # pause 变负、整幕瞬间倾泻（环月段一闪而过，看起来"没播"）。
    timeline = []
    acc = 0.0
    for chunk, per in schedule:
        acc += per
        timeline.append((chunk, acc))
    total_chunks = len(timeline)
    print('[player] %s: %d lines -> %d chunks, %.1f s total, loop=%d'
          % (args.file, n, total_chunks, acc, args.loop), flush=True)

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
        for idx, (chunk, t_target) in enumerate(timeline):
            try:
                conn.sendall(('\n'.join(chunk) + '\n').encode('utf-8'))
            except Exception as e:
                print('[player] lap %d: send failed at %d/%d: %r'
                      % (lap, idx, total_chunks, e), flush=True)
                return
            sent += len(chunk)
            # 匀速节拍（扣除已耗时间，避免累计漂移）
            pause = t0 + t_target - time.time()
            if pause > 0:
                time.sleep(pause)
            if idx % max(1, total_chunks // 10) == 0:
                print('[player] lap %d %3d%% (%d/%d chunks)'
                      % (lap, 100 * idx // total_chunks, idx, total_chunks),
                      flush=True)
        print('[player] lap %d done: %d lines in %.1fs'
              % (lap, sent, time.time() - t0), flush=True)

    print('[player] finished, closing', flush=True)
    time.sleep(2.0)


if __name__ == '__main__':
    main()
