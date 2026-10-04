#!/usr/bin/env python3
# queue_calc.py — 排队论计算器（M/M/1、M/M/c、单位换算）
# 出自: notes/section-2.6.4-排队论计算器.md 块3/4/5
#
# 用法:
#   ./queue_calc.py                 # 跑笔记里的示例（λ=7000, μ=10000）
#   ./queue_calc.py 7000 10000      # 自定义 λ/μ（同单位即可）
#   ./queue_calc.py 7000 10000 8    # 第三参数=通道数 c → 走 M/M/c 拐点扫描

import math
import sys


def to_per_second(rate, unit):
    """统一量纲：rate 带单位 ('s'/'ms'/'us') → 每秒。出自块5"""
    return rate * {"s": 1, "ms": 1e3, "us": 1e6}[unit]


def mm1(lam, mu):
    """M/M/1：返回 rho, L, W, Wq（W/Wq 单位 = 1/μ 的时间单位）"""
    rho = lam / mu
    L = rho / (1 - rho)          # 系统内平均任务数
    W = 1 / (mu - lam)           # 平均逗留时间
    Wq = rho / (mu - lam)        # 平均排队时间 = W - 1/μ
    return rho, L, W, Wq


def erlang_c(lam, mu, c):
    """M/M/c 的 Erlang-C：等待概率 P(wait>0)"""
    a = lam / mu                      # 话务量（Erlang）
    rho = a / c                       # 单通道利用率
    if rho >= 1:
        return 1.0, rho
    # P0 = [ Σ_{k=0}^{c-1} a^k/k! + a^c/(c!(1-rho)) ]^{-1}
    s = sum(a**k / math.factorial(k) for k in range(c))
    s += a**c / (math.factorial(c) * (1 - rho))
    p0 = 1.0 / s
    pw = (a**c / (math.factorial(c) * (1 - rho))) * p0
    return pw, rho


def mm_c(lam, mu, c):
    """M/M/c：返回 rho, Wq, W（单位 = 1/μ 的时间单位）"""
    pw, rho = erlang_c(lam, mu, c)
    if rho >= 1:
        return rho, math.inf, math.inf
    Wq = pw / (c * mu - lam)
    W = Wq + 1 / mu
    return rho, Wq, W


def main():
    lam = float(sys.argv[1]) if len(sys.argv) > 1 else 7000
    mu = float(sys.argv[2]) if len(sys.argv) > 2 else 10_000

    if len(sys.argv) <= 3:
        # ---- 块3：M/M/1 示例 ----
        rho, L, W, Wq = mm1(lam, mu)
        print(f"λ={lam:.0f}  μ={mu:.0f}")
        print(f"rho = {rho:.2f}   —— 先看这个，过 0.7 立刻紧张")
        print(f"L   = {L:.2f}   —— 系统内平均任务数")
        print(f"W   = {W * 1e3:.3f} ms —— 平均逗留时间")
        print(f"Wq  = {Wq * 1e3:.3f} ms —— 平均排队时间（W - 1/μ）")
    else:
        # ---- 块4：M/M/c 拐点扫描 ----
        c = int(sys.argv[3])
        print(f"M/M/{c} 扫描（λ = rho × c × μ，μ={mu:.0f}）")
        print(f"{'rho':>5} {'Wq(ms)':>10} {'W(ms)':>10}")
        for rho in [0.5, 0.6, 0.7, 0.75, 0.8]:
            r, Wq, W = mm_c(rho * c * mu, mu, c)
            print(f"{r:>5.2f} {Wq * 1e3:>10.4f} {W * 1e3:>10.4f}")
        print("knee ≈ 75~80%  →  告警 ≈ 65~70%  →  Ch12 压测验证")


if __name__ == "__main__":
    main()
