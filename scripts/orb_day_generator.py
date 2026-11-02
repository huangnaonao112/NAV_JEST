#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
orb_day_generator.py — 生成"一个接收站一天定位所需卫星"的 $ORB 报文文件

用途：模拟外部解算器的输出。读取 RINEX 3 广播星历（如 IGS BRDC），
对接收站按固定步长（默认 1 分钟）筛选全天空可见卫星（高度角 >= mask），
把每颗卫星在该时刻的开普勒根数写成 $ORB 帧，一天 1440 帧。

产物（NMEA 风格文本，可直接用 orb_file_player.py 回放）：
  $BEG,...          开始任务
  $ORB,...          每行一帧（= 该分钟可见星座）
  $END,...          结束清场

用法：
  python orb_day_generator.py --brdc BRDC00IGS_R_20241000000_01D_MN.rnx \
      --out playback/orb_day_full.txt
  # 抽稀（每 5 分钟一帧，288 帧）：
  python orb_day_generator.py --brdc ... --stride 5 --out playback/orb_day_5min.txt

注意：本脚本属于外部解算器角色，仿真软件本身不解析星历。
"""
import argparse
import math
import os
import re
import sys

MU = 398600.4418              # km^3/s^2
OMEGA_E = 7.2921150e-5        # rad/s
GPS_EPOCH_J2000 = 630763200.0 # 1980-01-06 -> 2000-01-01 12:00 (s)
MINUTES_PER_DAY = 1440

NUM_RE = re.compile(r'[-+]?\d+\.\d+[DdEe][-+]?\d+')


def nums(line):
    """提取一行内所有指数格式数值（历元整数会被自动跳过）"""
    return [float(m.group(0).replace('D', 'E').replace('d', 'e'))
            for m in NUM_RE.finditer(line)]


def parse_brdc(path):
    """解析 RINEX 3 NAV，返回记录列表（每条 = 某星某 toe 的根数）"""
    lines = open(path, encoding='utf-8', errors='replace').read().splitlines()
    hdr = next(i for i, l in enumerate(lines) if 'END OF HEADER' in l)
    recs = []
    i = hdr + 1
    while i + 7 < len(lines):
        l = lines[i]
        if len(l) > 3 and l[0] in 'GCE' and l[1:3].isdigit():
            b = lines[i:i + 8]
            try:
                v1, v2, v3, v4, v5, v6, v7 = (nums(x) for x in b[:7])
                sys_c = l[0]
                week = int(round(v6[2]))
                if sys_c == 'C':
                    week += 1356          # BDS 周归一到 GPS 周
                recs.append(dict(
                    sys=sys_c, prn=int(l[1:3]),
                    e=v3[1], sqrta=v3[3],
                    cus=v3[2], cuc=v3[0],
                    toe=v4[0], cic=v4[1], omg0=v4[2], cis=v4[3],
                    i0=v5[0], crc=v5[1], argp=v5[2], omgd=v5[3],
                    idot=v6[0],
                    crs=v2[1], dn=v2[2], m0=v2[3],
                    week=week, health=int(round(v7[1]))))
            except (IndexError, ValueError):
                pass
            i += 8
        else:
            i += 1
    return recs


def sat_ecef(rec, tow):
    """广播星历标准算法：ECEF(km)。与 ICD 一致（含谐波改正）。"""
    a = rec['sqrta'] ** 2 / 1000.0
    e = rec['e']
    n = math.sqrt(MU / a ** 3) + rec['dn']
    tk = tow - rec['toe']
    m = rec['m0'] + n * tk
    ecc = m
    for _ in range(30):
        d = (ecc - e * math.sin(ecc) - m) / (1 - e * math.cos(ecc))
        ecc -= d
        if abs(d) < 1e-13:
            break
    v = math.atan2(math.sqrt(1 - e * e) * math.sin(ecc), math.cos(ecc) - e)
    phi = v + rec['argp']
    sin2, cos2 = math.sin(2 * phi), math.cos(2 * phi)
    u = phi + rec['cus'] * sin2 + rec['cuc'] * cos2
    r = a * (1 - e * math.cos(ecc)) + rec['crs'] * sin2 + rec['crc'] * cos2
    inc = rec['i0'] + rec['idot'] * tk + rec['cis'] * sin2 + rec['cic'] * cos2
    xo, yo = r * math.cos(u), r * math.sin(u)
    omg = rec['omg0'] + (rec['omgd'] - OMEGA_E) * tk - OMEGA_E * rec['toe']
    x = xo * math.cos(omg) - yo * math.cos(inc) * math.sin(omg)
    y = xo * math.sin(omg) + yo * math.cos(inc) * math.cos(omg)
    z = yo * math.sin(inc)
    return x, y, z


def gmst_deg(t_j2000):
    """IAU 多项式近似（与本项目渲染侧 greenwichMeanSiderealAngle 同量级）"""
    t = t_j2000 / 3155760000.0   # 儒略世纪
    g = 67310.54841 + (876600 * 3600 + 8640184.812866) * t + 0.093104 * t * t
    return g % 360.0


def elevation_deg(rec, tow, week, lat, lon):
    # 广播星历标准算法(含 -Ωe·toe 与 (Ω̇-Ωe)·tk 项)的输出已是 ECEF——
    # 不能再做 GMST 旋转！(曾因多转一次恒星时角导致可见集整体错位 ~105°,
    # "北京可见"变成地球另一侧的错位集合)
    xe, ye, ze = sat_ecef(rec, tow)
    la, lo = math.radians(lat), math.radians(lon)
    sl, cl, so, co = math.sin(la), math.cos(la), math.sin(lo), math.cos(lo)
    east = -so * xe + co * ye
    north = -sl * co * xe - sl * so * ye + cl * ze
    up = cl * co * xe + cl * so * ye + sl * ze
    return math.degrees(math.atan2(up, math.hypot(east, north)))


def build_index(recs):
    """按 (sys, prn) 归组，toe 升序；回放中按 tow 选 toe<=tow 的最新记录"""
    groups = {}
    for r in recs:
        groups.setdefault((r['sys'], r['prn']), []).append(r)
    for k in groups:
        groups[k].sort(key=lambda r: r['toe'])
    return groups


def pick_record(group, tow):
    """取 toe <= tow 的最新记录；早于首条时用首条（Kepler 支持负 tk）"""
    chosen = group[0]
    for r in group:
        if r['toe'] <= tow:
            chosen = r
        else:
            break
    return chosen


def nmea_checksum(body):
    cs = 0
    for ch in body:
        cs ^= ord(ch)
    return '%02X' % cs


def frame_line(body):
    return '$%s*%s' % (body, nmea_checksum(body))


def hhmmss(tow):
    s = int(round(tow))
    return '%02d%02d%02d.000' % (s // 3600, (s % 3600) // 60, s % 60)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--brdc', required=True, help='RINEX 3 NAV 文件路径')
    ap.add_argument('--out', required=True, help='输出报文文件路径')
    ap.add_argument('--lat', type=float, default=39.9, help='接收站纬度 deg')
    ap.add_argument('--lon', type=float, default=116.4, help='接收站经度 deg')
    ap.add_argument('--mask', type=float, default=10.0, help='截止高度角 deg')
    ap.add_argument('--stride', type=int, default=1,
                    help='抽稀步长（分钟），1=全量 1440 帧')
    ap.add_argument('--test-type', type=int, default=2, help='$BEG 的场景类型')
    args = ap.parse_args()

    recs = parse_brdc(args.brdc)
    groups = build_index(recs)
    if not groups:
        sys.exit('no navigation records parsed')

    week = max(r['week'] for g in groups.values() for r in g)
    stride = max(1, args.stride)

    # ---- 第一遍：逐分钟算可见性，得到全天并集 U 与每分钟可见标记 ----
    union_keys = []          # [(sys, prn)] 固定顺序（渲染端集合稳定 → 快路径）
    union_seen = set()
    visible_flags = {}       # minute -> set of (sys, prn) 在该分钟参与定位
    for minute in range(0, MINUTES_PER_DAY, stride):
        tow = float(minute * 60)
        vis = set()
        for key, group in groups.items():
            rec = pick_record(group, tow)
            if rec['week'] != week or rec['health'] != 0:
                continue
            if elevation_deg(rec, tow, week, args.lat, args.lon) < args.mask:
                continue
            vis.add(key)
        visible_flags[minute] = vis
        for k in vis:
            if k not in union_seen:
                union_seen.add(k)
                union_keys.append(k)
    union_keys.sort()

    # ---- 第二遍：每帧固定发并集；health=0 表示该分钟参与定位，1 表示不参与 ----
    lines = [frame_line('BEG,%09.3f,%d' % (0.0, args.test_type))]
    # $PNT 声明接收站(模拟源)位置——2D 地图"定位到模拟源"与可见性筛选同源
    pnt = ('PNT,%09.3f,%.6f,%.6f,%.3f,%.6f,%.6f,%.3f,'
           '0.0000,0.0000,0.0000,0.0000,0.0000,0.0000'
           % (0.0, args.lat, args.lon, 0.05, args.lat, args.lon, 0.05))
    lines.append(frame_line(pnt))
    n_frames = 0
    sat_minutes = 0
    per_minute_counts = []
    for minute in range(0, MINUTES_PER_DAY, stride):
        tow = float(minute * 60)
        vis = visible_flags[minute]
        entries = []
        for key in union_keys:
            group = groups[key]
            rec = pick_record(group, tow)
            a = rec['sqrta'] ** 2 / 1000.0
            n = math.sqrt(MU / a ** 3) + rec['dn']
            m = (rec['m0'] + n * (tow - rec['toe'])) % (2.0 * math.pi)
            n_rev_day = n * 86400.0 / (2.0 * math.pi)
            # health 语义扩展：原始健康字之外，0=该分钟参与定位 / 1=不参与
            # （原始不健康的卫星本来就不在并集里——上面第一遍已过滤）
            health_flag = 0 if key in vis else 1
            entries.append((rec, a, m, n_rev_day, health_flag))
        if not entries:
            continue
        body = 'ORB,%s,%d,%.3f,%d' % (hhmmss(tow), week, tow, len(entries))
        fields = []
        for rec, a, m, n_rev, hflag in entries:
            fields += ['%d' % rec['prn'], rec['sys'],
                       '%.3f' % a, '%.9f' % rec['e'],
                       '%.6f' % math.degrees(rec['i0']),
                       '%.6f' % math.degrees(rec['omg0']),
                       '%.6f' % math.degrees(rec['argp']),
                       '%.6f' % math.degrees(m),
                       '%.8f' % n_rev,
                       '%d' % hflag]
        lines.append(frame_line(body + ',' + ','.join(fields)))
        n_frames += 1
        sat_minutes += len(vis)
        per_minute_counts.append(len(vis))

    lines.append(frame_line('END,%09.3f,%d' % (86399.0, args.test_type)))
    # 注: 上面 n_frames 只统计 $ORB 帧

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')

    size = os.path.getsize(args.out)
    avg = sat_minutes / max(1, n_frames)
    lo = min(per_minute_counts) if per_minute_counts else 0
    hi = max(per_minute_counts) if per_minute_counts else 0
    print('records=%d union_sats=%d frames=%d avg_visible/minute=%.1f min=%d max=%d'
          % (len(recs), len(union_keys), n_frames, avg, lo, hi))
    print('written %s (%.2f MB)' % (args.out, size / 1e6))


if __name__ == '__main__':
    main()
