#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
full_demo_generator.py — 三幕连续演示报文生成器

一个文件演示完全部场景（软件端按 $BEG/$END 分幕清场）：
  幕一  定位作业一天（96 个快照，每 15 分钟一帧）
        - $ORB: 真实星座 97 颗，tow 随快照推进 → 3D 卫星沿轨道转一天
        - $SAT: 星历解算的星下点（24~38 颗可见，随升降变化）→ 2D
        - $PNT: 模拟源/待测设备绕石家庄走 8 字作业圈（移动）
        - $DSP: 欺骗干扰 09:00-13:00 激活（携带轨道根数，随卫星移动）
        - $DEV/$ANT: 机柜/穹顶每 2 小时轮换（持续在亮）
  幕二  地月转移（设备方 $TRJ 真实轨迹，抽稀 41 点）
  幕三  环月（设备方 $MNT 轨迹 ×4 圈）

用法：
  python full_demo_generator.py --brdc 源星历.txt --seconds 120
  （seconds 只影响 orb_file_player.py 的播放时长建议，写进文件名提示）
"""
import argparse
import io
import math

MU = 398600.4418
OMEGA_E = 7.2921150e-5
BASE_LAT, BASE_LON = 38.0568, 114.3632   # 石家庄（sjz 瓦片 11-15 级最清晰）
MASK_DEG = 10.0
N_SNAP = 96                               # 一天 96 个快照（每 15 分钟）


def cs_reduce(body):
    c = 0
    for ch in body:
        c ^= ord(ch)
    return '%02X' % c


def frame(s):
    body = s.lstrip('$').split('*')[0]
    return '$%s*%s' % (body, cs_reduce(body))


def hhmmss(t):
    s = int(round(t)) % 86400
    return '%02d%02d%02d.000' % (s // 3600, (s % 3600) // 60, s % 60)


def parse_orb_frame(line):
    body = line[1:line.index('*')].split(',')
    sats = []
    for i in range(int(body[4])):
        b = 5 + i * 10
        sats.append(dict(sys=body[b + 1], prn=int(body[b]),
                         a=float(body[b + 2]), e=float(body[b + 3]),
                         i0=math.radians(float(body[b + 4])),
                         raan0=math.radians(float(body[b + 5])),
                         argp=math.radians(float(body[b + 6])),
                         M0=math.radians(float(body[b + 7])),
                         health=int(body[b + 9])))
    return int(body[2]), float(body[3]), sats


def sub_ecef(sat, tow):
    """帧根数语义：M0 + n·tow 推进后即 ECEF（Ω0 已吸收 GMST 偏置）"""
    a = sat['a']
    e = sat['e']
    n = math.sqrt(MU / a ** 3)
    M = sat['M0'] + n * tow
    E = M
    for _ in range(30):
        d = (E - e * math.sin(E) - M) / (1 - e * math.cos(E))
        E -= d
        if abs(d) < 1e-13:
            break
    nu = math.atan2(math.sqrt(1 - e * e) * math.sin(E), math.cos(E) - e)
    u = nu + sat['argp']
    r = a * (1 - e * math.cos(E))
    xo, yo = r * math.cos(u), r * math.sin(u)
    O, inc = sat['raan0'], sat['i0']
    x = xo * math.cos(O) - yo * math.cos(inc) * math.sin(O)
    y = xo * math.sin(O) + yo * math.cos(inc) * math.cos(O)
    z = yo * math.sin(inc)
    # 广播语义补地球自转：Ω0 对齐的是周起始惯性系，ECEF = 绕 Z 转
    # −Ωe·tow（与 C++ 渲染同一约定，实测 tow=0 帧位置即真值）。
    # 此前漏转，星下点/仰角随 tow 累积偏差（12h 达 180°），可见卫星数
    # 一天内疯摆（1~38），$SAT 星下点在 2D 图上漂移。
    w = -OMEGA_E * tow
    x, y = x * math.cos(w) - y * math.sin(w), x * math.sin(w) + y * math.cos(w)
    return x, y, z


def elevation(sat, tow, rec_lat, rec_lon):
    x, y, z = sub_ecef(sat, tow)
    r = math.sqrt(x * x + y * y + z * z)
    lon_s = math.atan2(y, x)
    lat_s = math.asin(z / r)
    la1, lo1 = math.radians(rec_lat), math.radians(rec_lon)
    d = math.acos(min(1.0, math.sin(la1) * math.sin(lat_s) +
                      math.cos(la1) * math.cos(lat_s) * math.cos(lon_s - lo1)))
    return math.degrees(math.atan((math.cos(d) - 6371.0 / r) / math.sin(d)))


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--orb-frame-file', default='playback/orb_day_5min.txt',
                    help='含 $ORB 帧的文件（星历来源）')
    ap.add_argument('--scenes', required=True,
                    help='设备方场景文件（提供 $TRJ/$MNT 素材）')
    ap.add_argument('--out', default='full_demo.txt')
    ap.add_argument('--play-seconds', type=int, default=300,
                    help='建议播放时长（写入注释）')
    args = ap.parse_args()

    lines = io.open(args.scenes, encoding='utf-8', errors='replace').read().splitlines()
    trj = [l for l in lines[60:2066] if l.startswith('$TRJ')][::7]
    mnt = [l for l in lines[2067:2092] if l.startswith('$MNT')]

    orb_line = next(l for l in io.open(args.orb_frame_file, encoding='utf-8')
                    if l.startswith('$ORB'))
    week, _, sats = parse_orb_frame(orb_line)

    out = []
    # ================= 幕一：定位作业一天 =================
    out.append(frame('BEG,000000.000,1'))
    out.append(frame('DEV,000001.000,' + ','.join(['1'] * 18)))
    out.append(frame('ANT,000001.000,' + ','.join(['1'] * 36)))
    out.append(frame('JAM,000002.000,%.6f,%.6f,50.0,45.0' % (BASE_LAT, BASE_LON)))
    out.append(frame('DUT,000002.000,1.5,0.2,2.3,0.1,3.1,0.05,0.8,0.1,1.2,0.08,2.5,0.12'))

    day = 86400.0
    # 欺骗源在整个演示期间固定（曾按每快照重新挑选 → DSP-1 每帧换轨道
    # → 欺骗卫星瞬移抽搐）。选定一次：中轨取仰角最接近 45° 的 MEO，
    # 高轨取可见 IGSO/GEO。
    src1 = min((s for s in sats if 20000 <= s['a'] <= 30000 and s['health'] == 0),
               key=lambda s: abs(elevation(s, 0.0, BASE_LAT, BASE_LON) - 45.0),
               default=None)
    src2 = max((s for s in sats if s['a'] > 35000 and s['health'] == 0),
               key=lambda s: elevation(s, 0.0, BASE_LAT, BASE_LON),
               default=None)
    for i in range(N_SNAP):
        tow = i * day / N_SNAP
        vis = []
        for s in sats:
            if s['health'] != 0:
                continue
            if elevation(s, tow, BASE_LAT, BASE_LON) >= MASK_DEG:
                x, y, z = sub_ecef(s, tow)
                r = math.sqrt(x * x + y * y + z * z)
                vis.append((s, math.degrees(math.asin(z / r)),
                            math.degrees(math.atan2(y, x))))
        vis.sort(key=lambda t: (t[0]['sys'], t[0]['prn']))

        # $ORB：tow 推进 + 每星 M0 推进 → 3D 星座沿轨道转一天
        fields = ['%d' % len(sats)]
        for s in sats:
            n_deg = math.sqrt(MU / (s['a'] ** 3)) * 180.0 / math.pi
            # 参与定位（health=0）逐帧重估：仰角≥遮蔽角才参与，其余置 1。
            # 真实 GNSS 行为：单星数小时升落更替，同时可见数量基本稳定。
            # 与 $SAT 星下点用同一筛选，保证 2D/3D 可见集一致。
            if s['health'] == 0:
                h = 0 if elevation(s, tow, BASE_LAT, BASE_LON) >= MASK_DEG else 1
            else:
                h = s['health']
            fields += ['%d' % s['prn'], s['sys'],
                       '%.3f' % s['a'], '%.7f' % s['e'],
                       '%.4f' % math.degrees(s['i0']),
                       '%.4f' % math.degrees(s['raan0']),
                       '%.4f' % math.degrees(s['argp']),
                       '%.4f' % ((math.degrees(s['M0']) + n_deg * tow) % 360.0),
                       '1.0027', '%d' % h]
        out.append(frame('ORB,%s,%d,%.3f,%s' % (hhmmss(tow), week, tow, ','.join(fields))))

        # $SAT：可见卫星星下点（真实解算，在动）→ 2D
        fields = ['%d' % len(vis)]
        for s, lat_s, lon_s in vis:
            fields += ['%s%02d' % (s['sys'], s['prn']),
                       '%.2f' % lat_s, '%.2f' % lon_s]
        out.append(frame('SAT,%s,%s' % (hhmmss(tow), ','.join(fields))))

        # $PNT：模拟源/待测设备绕石家庄走 8 字作业圈（移动）
        ph = 2 * math.pi * i / N_SNAP
        lat = BASE_LAT + 0.020 * math.sin(ph)
        lon = BASE_LON + 0.028 * math.sin(2 * ph)
        alt = 100.0 + 8.0 * math.sin(ph)
        out.append(frame('PNT,%s,%.6f,%.6f,%.2f,%.6f,%.6f,%.2f,0,0,0,0,0,0'
                         % (hhmmss(tow), lat, lon, alt,
                            lat + 0.0002, lon + 0.0003, alt + 0.4)))

        # $DSP：欺骗干扰 09:00-13:00 激活（M0 随 tow 推进 → 随卫星移动）
        active = 1 if 32400.0 <= tow <= 46800.0 else 0
        if src1 and src2:
            n1 = math.sqrt(MU / (src1['a'] ** 3)) * 180.0 / math.pi
            n2 = math.sqrt(MU / (src2['a'] ** 3)) * 180.0 / math.pi
            e1 = '%d,%.3f,%.7f,%.4f,%.4f,%.4f,%.4f,%d' % (
                1, src1['a'], src1['e'], math.degrees(src1['i0']),
                (math.degrees(src1['raan0']) + 8.0) % 360.0,
                math.degrees(src1['argp']),
                (math.degrees(src1['M0']) + n1 * tow) % 360.0, active)
            e2 = '%d,%.3f,%.7f,%.4f,%.4f,%.4f,%.4f,%d' % (
                2, src2['a'], src2['e'], math.degrees(src2['i0']),
                (math.degrees(src2['raan0']) - 10.0) % 360.0,
                math.degrees(src2['argp']),
                (math.degrees(src2['M0']) + n2 * tow) % 360.0, active)
            out.append(frame('DSP,%s,2,%s,%s' % (hhmmss(tow), e1, e2)))

        # 机柜/穹顶每 2 小时轮换（持续在亮）
        if i % 8 == 0:
            out.append(frame('DEV,%s,%s' % (hhmmss(tow), ','.join(['1'] * 18))))
            ant = [1 if (j // 6) % 2 == (i // 8) % 2 else 0 for j in range(36)]
            out.append(frame('ANT,%s,%s' % (hhmmss(tow), ','.join(str(v) for v in ant))))

    out.append(frame('END,86400.000,1'))

    # ================= 幕二：地月转移 =================
    out.append(frame('BEG,090000.000,2'))
    out.extend(trj)
    out.append(frame('END,100000.000,2'))

    # ================= 幕三：环月 ×4 圈 =================
    out.append(frame('BEG,110000.000,3'))
    for p_i in range(4):
        for l in mnt:
            b = l[1:l.index('*')].split(',')
            t0 = int(b[1][:2]) * 3600 + int(b[1][2:4]) * 60 + int(b[1][4:6])
            b[1] = '%09.3f' % (t0 + p_i * 600)
            out.append(frame(','.join(b)))
    out.append(frame('END,120000.000,3'))

    io.open(args.out, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
    counts = []
    for i in range(0, N_SNAP, 8):
        tow = i * day / N_SNAP
        counts.append(sum(1 for s in sats if s['health'] == 0 and
                          elevation(s, tow, BASE_LAT, BASE_LON) >= MASK_DEG))
    print('幕一定位快照=%d 可见卫星=%d~%d(定位所需) 总行数=%d' %
          (N_SNAP, min(counts), max(counts), len(out)))
    print('建议播放时长: --seconds %d' % args.play_seconds)


if __name__ == '__main__':
    main()
