#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
orb_common.py — 报文生成器共享物理/工具模块

两个生成器（orb_day_generator / full_demo_generator）共用的时间、校验和、
Kepler 迭代与坐标约定都收拢在本模块，避免"同一公式两份拷贝各自演化"
（曾因 full_demo 的 sub_ecef 漏了 -Ωe·tow 修正而与 C++ 渲染约定漂移，
星下点/仰角随 tow 累积偏差、12h 达 180°）。

$ORB 帧字段布局（orb_day 写 / full_demo 读，本模块是唯一契约点）：
  $ORB,HHMMSS.SSS,week,tow,n,然后每星 10 个字段：
    prn, sys, a(km), e, i0(deg), raan0(deg), argp(deg), M0(deg),
    n_rev_day(revs/day，full_demo 恒填 1.0027 仅作占位), health(0=参与定位)
  详见 docs/轨道参数报文接口.md

坐标约定（与 C++ 渲染严格一致，改动前先读 docs/坐标系与尺度规范.md）：
  - 广播根数 Ω0 对齐"周起始"惯性系；惯性推进位置绕 Z 转 -Ωe·tow 才是 ECEF
  - 简化二体（本模块 sub_ecef）与 C++ propagateKepler+rotateZ(-Ωe·tow) 等价
"""
import math

MU = 398600.4418              # km^3/s^2
OMEGA_E = 7.2921150e-5        # rad/s，地球自转角速率（与 C++ OrbitalMechanics.h 同值）


def nmea_checksum(body):
    """NMEA 风格 XOR 校验，两位大写十六进制"""
    cs = 0
    for ch in body:
        cs ^= ord(ch)
    return '%02X' % cs


def frame_line(body):
    """包一层 $...*CS"""
    return '$%s*%s' % (body, nmea_checksum(body))


def hhmmss(tow):
    """周内秒 -> HHMMSS.000（超一天值按日取模）"""
    s = int(round(tow)) % 86400
    return '%02d%02d%02d.000' % (s // 3600, (s % 3600) // 60, s % 60)


def parse_orb_frame(line):
    """解析 $ORB 帧文本 -> (week, tow, sats[dict])。
    字段布局是 orb_day(写) 与 full_demo(读) 的共享契约，见模块 docstring。"""
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


def solve_kepler(m, e, tol=1e-13, max_iter=30):
    """牛顿迭代解开普勒方程 M = E - e·sin(E)，返回偏近点角 E（弧度）"""
    ecc = m
    for _ in range(max_iter):
        d = (ecc - e * math.sin(ecc) - m) / (1 - e * math.cos(ecc))
        ecc -= d
        if abs(d) < tol:
            break
    return ecc


def sub_ecef(sat, tow):
    """简化二体星历 -> ECEF(km)。full_demo 生成器专用（元素为静态快照）。

    帧根数语义：M0 + n·tow 惯性推进后，绕 Z 转 -Ωe·tow 才是 ECEF
    （Ω0 对齐周起始惯性系，与 C++ propagateKepler + rotateZ(-Ωe·tow) 同约定）。
    sat 字典键：a, e, i0(rad), raan0(rad), argp(rad), M0(rad)
    """
    a = sat['a']
    e = sat['e']
    n = math.sqrt(MU / a ** 3)
    m = sat['M0'] + n * tow
    ecc = solve_kepler(m, e)
    nu = math.atan2(math.sqrt(1 - e * e) * math.sin(ecc), math.cos(ecc) - e)
    u = nu + sat['argp']
    r = a * (1 - e * math.cos(ecc))
    xo, yo = r * math.cos(u), r * math.sin(u)
    o, inc = sat['raan0'], sat['i0']
    x = xo * math.cos(o) - yo * math.cos(inc) * math.sin(o)
    y = xo * math.sin(o) + yo * math.cos(inc) * math.cos(o)
    z = yo * math.sin(inc)
    w = -OMEGA_E * tow
    x, y = x * math.cos(w) - y * math.sin(w), x * math.sin(w) + y * math.cos(w)
    return x, y, z


def elevation_deg(x, y, z, lat, lon):
    """接收站仰角(deg)，ECEF(km) 用 ENU 投影精确计算（orb_day 用）"""
    la, lo = math.radians(lat), math.radians(lon)
    sl, cl, so, co = math.sin(la), math.cos(la), math.sin(lo), math.cos(lo)
    east = -so * x + co * y
    north = -sl * co * x - sl * so * y + cl * z
    up = cl * co * x + cl * so * y + sl * z
    return math.degrees(math.atan2(up, math.hypot(east, north)))


def elevation_horizon_deg(sat, tow, rec_lat, rec_lon):
    """接收站仰角(deg)，球面近似 horizon 公式（full_demo 沿用，与
    elevation_deg 在球形地球假设下等价，量级差 <0.5°）"""
    x, y, z = sub_ecef(sat, tow)
    r = math.sqrt(x * x + y * y + z * z)
    lon_s = math.atan2(y, x)
    lat_s = math.asin(z / r)
    la1, lo1 = math.radians(rec_lat), math.radians(rec_lon)
    d = math.acos(min(1.0, math.sin(la1) * math.sin(lat_s) +
                      math.cos(la1) * math.cos(lat_s) * math.cos(lon_s - lo1)))
    return math.degrees(math.atan((math.cos(d) - 6371.0 / r) / math.sin(d)))
