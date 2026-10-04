#!/bin/bash
# layers-observability.sh — 观测工具四层递进（计数器→指标→剖析→追踪）
# 出自: notes/section-1.7-观测工具四层递进.md 块4
# 用法: ./layers-observability.sh [pid] [网卡名]
set -u

PID="${1:-}"
NIC="${2:-eth0}"

echo "===== ① 计数器 Counters — 「有没有异常」 ====="
echo "--- mpstat -P ALL 1（Ctrl-C 停）"
mpstat -P ALL 1 3
echo "--- ethtool -S $NIC | 只看非零丢包/错误计数"
ethtool -S "$NIC" 2>/dev/null | grep -E "drop|err|miss" | grep -v ": 0$" || echo "（无非零丢包/错误）"
echo "--- perf stat -e cache-misses（需要 perf 权限，当前 paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)）"
if [ -n "$PID" ]; then
    perf stat -e cache-misses,cache-references -p "$PID" -- sleep 5
else
    perf stat -e cache-misses,cache-references -- sleep 5
fi

echo
echo "===== ② 指标 Metrics ====="
echo "应用内 histogram / P99 曲线（或 Prometheus）——见 ch02 分层埋点 demo"

echo
echo "===== ③ 剖析 Profiling — 「哪段代码宽」 ====="
if [ -n "$PID" ]; then
    echo "perf record -g -p $PID -- sleep 30 && perf report"
    perf record -g -p "$PID" -- sleep 30 && perf report --stdio | head -30
else
    echo "（未给 pid，跳过；用法: $0 <pid> [nic]）"
fi

echo
echo "===== ④ 追踪 Tracing — 「这一笔逐步花在哪」 ====="
echo "bpftrace / ftrace：订单 ID 关联 RX→decode→send 各段 Δt（脚本见 Ch15/06.7 模块）"
