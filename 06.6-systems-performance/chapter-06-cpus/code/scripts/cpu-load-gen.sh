#!/bin/bash
# cpu-load-gen.sh — CPU 负载生成三式（ch06 §6.8 实验工具）
# 出自: notes/section-6.8-实验工具.md 块6/7/8
# 用法: ./cpu-load-gen.sh busy [核号]     —— 裸 busy loop（无依赖）
#       ./cpu-load-gen.sh sysbench        —— sysbench cpu 质数（需安装）
#       ./cpu-load-gen.sh stress          —— stress-ng 五连（需安装）
set -u

MODE="${1:?用法: $0 busy|sysbench|stress [核号]}"

case "$MODE" in
busy)
    CORE="${2:-}"
    echo "启动 busy loop（用完记得 kill）"
    if [ -n "$CORE" ]; then
        taskset -c "$CORE" bash -c 'while :; do :; done' &
        echo "已绑核 $CORE，pid=$!"
    else
        while :; do :; done &
        echo "pid=$!"
    fi
    echo "清理: kill %1 或 pkill -f 'while :; do :; done'"
    ;;
sysbench)
    command -v sysbench >/dev/null || { echo "未安装: sudo apt install sysbench"; exit 1; }
    sysbench --num-threads=8 --test=cpu --cpu-max-prime=100000 run
    ;;
stress)
    command -v stress-ng >/dev/null || { echo "未安装: sudo apt install stress-ng"; exit 1; }
    echo "--- CPU 算力（matrixprod）";        stress-ng --cpu 8 --cpu-method matrixprod --timeout 60s
    echo "--- cache 压力";                    stress-ng --cache 4 --timeout 60s
    echo "--- 内存带宽";                      stress-ng --vm 4 --vm-bytes 1G --timeout 60s
    echo "--- 调度压力（频繁切换）";          stress-ng --sched 8 --timeout 60s
    echo "--- 矩阵（SIMD 路径）";             stress-ng --matrix 4 --matrix-size 128 --timeout 60s
    ;;
esac
