#!/bin/bash
# perf-full-workflow.sh — 计数→on-CPU→off-CPU 三段式排查（ch13 §13.10 块4）
# 用法: ./perf-full-workflow.sh <进程名>
# 思想: 先计数排调度，再 on-CPU 排计算，最后 off-CPU 找阻塞——顺序不能乱
set -u

PROC="${1:?用法: $0 <进程名>}"
PID=$(pidof "$PROC" | awk '{print $1}')
[ -n "$PID" ] || { echo "进程未找到: $PROC"; exit 1; }

echo "===== S2 ① 计数先行（找方向，别急着采样） ====="
echo "perf stat -e cycles,instructions,LLC-load-misses,context-switches -p $PID -I 1000 -- sleep 60"
echo "→ IPC 正常但 context-switches 周期性尖 → 调度问题，CPU 图白采"

echo
echo "===== S3 ② on-CPU 长采（排除计算慢） ====="
echo "perf record -F 499 -g -p $PID -C 3 -- sleep 120"
echo "perf script | stackcollapse-perf.pl | flamegraph.pl > $PROC-cpu.svg"
echo "→ 无异常热点 → 排除计算慢"

echo
echo "===== S4 ③ off-CPU 补刀（找阻塞） ====="
echo "sudo offcputime-bpfcc -p $PID 60 > off.txt"
echo "→ 5ms 尖刺=epoll_wait 唤醒后等 CPU → runqlat 迁核审计"
echo "  （offcputime 栈解读: 06.7-bpf-observability/02-bpf-performance-tools/chapter-06-cpus/）"
