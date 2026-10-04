#!/bin/bash
# ipc-measure.sh — IPC 测量（每指令周期数，CPU 效率的第一指标）
# 出自: notes/section-6.1-6.3 块1、section-6.4 块2
# 用法: ./ipc-measure.sh [程序路径]   # 不给程序则测全系统 1 秒
set -u

PARA=$(cat /proc/sys/kernel/perf_event_paranoid)
[ "$PARA" -le 2 ] || echo "⚠ perf_event_paranoid=$PARA，非 root 可能无数据（sudo sysctl kernel.perf_event_paranoid=2）"

if [ $# -ge 1 ]; then
    echo "===== 测程序: $* ====="
    perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses -- "$@"
else
    echo "===== 全系统 1 秒 ====="
    perf stat -e cycles,instructions,cache-misses,cache-references -- sleep 1
fi

cat <<'EOF'

读数：
  IPC = instructions / cycles
    IPC < 1    → 严重 stall（等内存/等分支）——查 cache-misses 占比
    IPC 1~2    → 一般
    IPC > 3    → 接近纯计算上限（向量化友好）
  cache-misses / cache-references > 5% → 访存是瓶颈（→ ch02 双视角、ch06 §6.4）
EOF
