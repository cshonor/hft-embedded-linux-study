#!/usr/bin/env python3
# scaling_calc.py — Amdahl 定律 + USL（通用扩展定律）拟合
# 出自: notes/section-2.7.1-阿姆达尔与USL.md 块6/7
#
# 用法:
#   ./scaling_calc.py            # Amdahl 示例 + USL 拟合示例（需 numpy/scipy）
#   ./scaling_calc.py --amdahl   # 只跑 Amdahl（无第三方依赖）

import sys


def amdahl_speedup(f: float, n: int) -> float:
    """f=串行占比 0~1, n=核数 → 加速比 S(n)。出自块6"""
    return 1.0 / (f + (1 - f) / n)


def amdahl_latency(T0_us: float, f: float, n: int) -> float:
    return T0_us / amdahl_speedup(f, n)


def demo_amdahl():
    # 块6 示例：f=10%, 8 核 → 1/(0.1+0.9/8) = 4.71
    # （笔记注释写的 ~4.3 是笔误，正确值 4.71，已在笔记中修正）
    print(f"S(8, f=0.1)      = {amdahl_speedup(0.1, 8):.2f}   （正确值 4.71）")
    print(f"S_max = 1/f      = {1 / 0.1:.0f}          （串行段卡死上限）")
    print(f"latency(10μs→∞)  ≈ {amdahl_latency(10.0, 0.1, 9999):.2f} μs （理想下限）")


def demo_usl():
    # 块7：USL 拟合（需要 numpy/scipy）
    try:
        import numpy as np
        from scipy.optimize import curve_fit
    except ImportError:
        print("（跳过 USL：需要 numpy/scipy —— pip3 install numpy scipy）")
        return

    def usl(n, alpha, beta):
        n = np.asarray(n, dtype=float)
        return n / (1 + alpha * (n - 1) + beta * n * (n - 1))

    # 压测：核数 vs 相对单核吞吐（示例数据，换成你的 orders/s 或 tick/s）
    N = np.array([1, 2, 4, 8, 16, 32])
    X = np.array([1.0, 1.8, 3.2, 4.5, 4.8, 4.2])  # 16 核后反降 → USL 典型

    (alpha, beta), _ = curve_fit(usl, N, X, p0=[0.01, 0.001], bounds=(0, np.inf))
    print(f"α={alpha:.4f}（竞争）  β={beta:.4f}（一致性/回滚）")
    print(f"N=8  预测 {usl(8, alpha, beta):.2f}  实测 {X[3]}")
    # 拐点：dX/dN = 0 → N* = sqrt((1-α)/β)
    if beta > 0:
        print(f"拐点 N* ≈ {((1 - alpha) / beta) ** 0.5:.1f} 核 —— 超过这个核数吞吐反降")


if __name__ == "__main__":
    demo_amdahl()
    if "--amdahl" not in sys.argv:
        print()
        demo_usl()
