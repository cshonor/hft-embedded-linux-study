#!/bin/bash
# perf-stat-ab.sh — perf stat 三用法：IPC 趋势 / 只看用户态 / A/B 实验纪律（ch13 §13.8 块15）
# 用法: ./perf-stat-ab.sh trend         —— 每 CPU 每秒 IPC（热核审计）
#       ./perf-stat-ab.sh user <进程名>  —— 只看用户态
#       ./perf-stat-ab.sh ab <旧程序> <新程序>  —— A/B 各跑 5 次
set -u

case "${1:-}" in
trend)
    perf stat -e cycles,instructions -I 1000 -a -- sleep 5
    ;;
user)
    PID=$(pidof "${2:?缺进程名}" | awk '{print $1}')
    perf stat -e cycles,instructions -u -p "$PID" -- sleep 10
    ;;
ab)
    OLD="${2:?用法: $0 ab <旧程序> <新程序>}"
    NEW="${3:?缺新程序}"
    perf stat -r 5 -e cycles,instructions,LLC-load-misses -- "$OLD" > old.txt 2>&1
    perf stat -r 5 -e cycles,instructions,LLC-load-misses -- "$NEW" > new.txt 2>&1
    echo "===== 对比（-r 5 取均值，看 ± 波动） ====="
    diff <(grep -E "cycles|instructions|LLC|seconds" old.txt) \
         <(grep -E "cycles|instructions|LLC|seconds" new.txt) || true
    echo "（结果在 old.txt / new.txt；波动 > 差值 = 结论不成立，先排干扰再测）"
    ;;
*)
    echo "用法: $0 trend | user <进程名> | ab <旧> <新>"; exit 1;;
esac
