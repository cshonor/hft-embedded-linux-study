#!/bin/bash
# observer-effect-bench.sh — 观测开销自测（Observing Observability）
# 出自: notes/section-4.6-观测的观测Observing-Observability.md 块7
# 用法: ./observer-effect-bench.sh <你的benchmark命令> <PID>
# 原理: 差值 = 观测开销（对被测指标的影响，不是工具的 CPU 占用）
set -u

BENCH="${1:?用法: $0 <benchmark命令> <PID>}"
PID="${2:?缺 PID}"

echo "===== ① 基线：不挂任何观测 ====="
$BENCH --duration 60 --output baseline.json 2>/dev/null || $BENCH

echo
echo "===== ② 挂上观测（perf 99Hz 采样 + bpftrace 聚合） ====="
perf record -F 99 -g -p "$PID" -o perf.data &
PERF_PID=$!
sudo bpftrace -e 'tracepoint:block:block_rq_issue { @[comm] = count(); }' &
BPF_PID=$!
sleep 1
$BENCH --duration 60 --output with_tools.json 2>/dev/null || $BENCH

kill $PERF_PID 2>/dev/null
sudo kill $BPF_PID 2>/dev/null

cat <<'EOF'

===== ③ 对比 P50/P99/吞吐 =====
  baseline.json vs with_tools.json
  差值 = 观测开销（对被测指标的影响，不是工具的 CPU 占用）
  经验：perf -F 99 通常 <3%；bpftrace 聚合 <1%；strace -p 可以到 10-100×
EOF
