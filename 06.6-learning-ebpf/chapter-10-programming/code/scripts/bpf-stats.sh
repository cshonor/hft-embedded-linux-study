#!/bin/bash
# bpf-stats.sh — BPF 程序自身性能统计（ch10 §10.2 块3）
# 出自: notes/10.2_测试统计与程序协作.md
# 用法: sudo ./bpf-stats.sh [秒数]
# 需要 root（写 sysctl）
set -u

SECS="${1:-30}"

echo "===== ① 开启统计 ====="
sysctl -w kernel.bpf_stats_enabled=1

echo "采 $SECS 秒…（期间让你的 BPF 程序有负载）"
sleep "$SECS"

echo
echo "===== ② 读数：每个程序的 run_time_ns / run_cnt ====="
bpftool prog list

echo
echo "===== ③ 关闭（统计本身有开销，用完必关） ====="
sysctl -w kernel.bpf_stats_enabled=0

cat <<'EOF'
读法：
  run_time_ns / run_cnt = 单次执行平均耗时
  热路径程序（XDP/raw_tp）单次应 < 1µs；
  超过就看是不是 map 操作太多、probe_read 太大、循环太重
EOF
