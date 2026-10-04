#!/bin/bash
# hft-10s-triage.sh — HFT 10 秒三连（第一现场排查）
# 出自: notes/section-1.9-1.11-云计算方法论与案例.md 块9
# 用法: ./hft-10s-triage.sh <策略进程名或PID>
set -u

PROC="${1:?用法: $0 <策略进程名或PID>}"

echo "===== ① 网卡 IRQ 分布（看 irqbalance 有没有把 IRQ 撒到策略核上） ====="
grep -E 'eth|enp' /proc/interrupts | head

echo
echo "===== ② mpstat -P ALL（usr/soft 占用，3 秒窗口） ====="
mpstat -P ALL 1 3

echo
echo "===== ③ 策略进程的核亲和性 ====="
PID=$(pgrep -f "$PROC" | head -1)
if [ -n "$PID" ]; then
    taskset -cp "$PID"
    echo
    echo "===== ④ perf top（生产用低频/短窗口，Ctrl-C 退出） ====="
    echo "（需要 perf_event_paranoid<=2 或 root；本机当前: $(cat /proc/sys/kernel/perf_event_paranoid)）"
    perf top -p "$PID"
else
    echo "未找到进程: $PROC"
fi
