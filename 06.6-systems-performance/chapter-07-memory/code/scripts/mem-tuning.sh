#!/bin/bash
# mem-tuning.sh — 内存调优两板斧：关 swap / 换 TCMalloc（ch07 §7.6）
# 出自: notes/section-7.6-调优指南.md 块6/7
# 用法: ./mem-tuning.sh swapoff        —— 关 swap（需 root，临时）
#       ./mem-tuning.sh tcmalloc <程序> —— LD_PRELOAD TCMalloc 跑
set -u

case "${1:-}" in
swapoff)
    echo "当前 swap:"; swapon --show
    echo "执行: sudo swapoff -a（持久化：/etc/fstab 注释掉 swap 行）"
    echo "替代（不折腾分区的软方案）: sudo sysctl vm.swappiness=1"
    ;;
tcmalloc)
    shift
    TC=/usr/lib/x86_64-linux-gnu/libtcmalloc.so.4
    [ -f "$TC" ] || { echo "未安装: sudo apt install libgoogle-perftools4"; exit 1; }
    echo "LD_PRELOAD=$TC $*"
    echo "（benchmark 验证后再上生产；对比指标：P99 malloc 耗时、缺页率）"
    LD_PRELOAD="$TC" "$@"
    ;;
*)
    echo "用法: $0 swapoff | tcmalloc <程序>"; exit 1;;
esac
