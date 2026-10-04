#!/bin/bash
# mem-observability.sh — 内存观测三式：swap 监控 / 缺页热点 / direct reclaim
# 出自: notes/section-7.5-观测工具.md 块5
# 用法: ./mem-observability.sh <进程名>
set -u

PROC="${1:-}"

echo "===== ① Swap 持续监控（si so 两列，恒 0 才正常） ====="
echo "vmstat 1 | awk 'NR>2 {print \$7,\$8}'   # Ctrl-C 停"
vmstat 1 5 | awk 'NR>2 {print "si="$7, "so="$8}'

if [ -n "$PROC" ]; then
    PID=$(pidof "$PROC" | awk '{print $1}')
    echo
    echo "===== ② 缺页热点（开发/压测环境，pid=$PID） ====="
    echo "perf record -e major-faults -g -p $PID -- sleep 30"
    echo "perf script | stackcollapse-perf.pl | flamegraph.pl > major-fault.svg"
    echo "（major fault = 从磁盘取页，每做一次就是几 ms 的 tail）"
fi

echo
echo "===== ③ direct reclaim 受害者（需 root） ====="
echo "sudo drsnoop-bpfcc"
echo "# TIME(s)  COMM   PID    LAT(ms)   ← 分配路径等回收的逐次记录"
echo "# HFT 热路径出现任何一条 = 事故（预防：关 swap、预分配+mlock、调 watermark）"
