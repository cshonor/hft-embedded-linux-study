#!/bin/bash
# perf-cheatsheet.sh — perf 单行命令速查（ch13 §13.1-13.2 块5）
# 用法: ./perf-cheatsheet.sh <进程名>
set -u

PROC="${1:?用法: $0 <进程名>}"
PID=$(pidof "$PROC" | awk '{print $1}')
[ -n "$PID" ] || { echo "进程未找到: $PROC"; exit 1; }
echo "目标: $PROC (pid=$PID)；perf_event_paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)"

echo
echo "--- 健康速查（计数流，开销≈0，生产可跑） ---"
echo "perf stat -e cycles,instructions,cache-misses,branch-misses -- sleep 1"
echo "perf stat -p $PID -- sleep 5"
echo "perf stat -e cycles,instructions,page-faults,major-faults -p $PID -- sleep 10"

echo
echo "--- CPU 热点（采样流，限 PID + 限时长） ---"
echo "perf record -F 99 -g -p $PID -- sleep 30"
echo "perf report --stdio --no-children | head -40"

echo
echo "--- 火焰图管道（需 FlameGraph 仓库） ---"
echo "perf script | stackcollapse-perf.pl | flamegraph.pl > strategy.svg"

echo
echo "--- 实时 top（开发机） ---"
echo "perf top -p $PID"

echo
echo "--- syscall 追踪（开发/debug，限时长） ---"
echo "perf trace -p $PID -- sleep 5"

echo
echo "--- 事件勘探 ---"
perf list 2>/dev/null | grep -E 'cache|fault|sched' | head -10
echo "perf list pmu   # 列出本机 PMU"
