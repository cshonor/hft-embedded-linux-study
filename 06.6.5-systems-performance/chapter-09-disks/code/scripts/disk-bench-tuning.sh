#!/bin/bash
# disk-bench-tuning.sh — 磁盘基准与调优（ch09 §9.7-9.9 块6/7）
# 用法: ./disk-bench-tuning.sh bench [设备]   —— fio 4k 随机读 QD1 延迟验收
#       ./disk-bench-tuning.sh ionice <pid>   —— 备份类进程降为 idle I/O 类
set -u

case "${1:-}" in
bench)
    DEV="${2:-/dev/nvme0n1}"
    command -v fio >/dev/null || { echo "未安装: sudo apt install fio"; exit 1; }
    echo "⚠ 直接测裸设备 $DEV 是只读测试（randread），不破坏数据"
    fio --name=lat4k --filename="$DEV" --direct=1 --rw=randread \
        --bs=4k --iodepth=1 --runtime=60 --time_based \
        --percentile_list=50:99:99.9:99.99 --readonly
    echo
    echo "HFT 日志盘验收：另跑 --rw=randwrite（写路径 + GC 行为）"
    echo "快速抽查: ioping -c 10 /var/log/hft（ioping 本机未装: sudo apt install ioping）"
    ;;
ionice)
    PID="${2:?用法: $0 ionice <pid>}"
    ionice -c 3 -p "$PID" && echo "pid $PID 已设为 idle I/O 类（队列空闲才发 I/O）"
    echo "cgroup v2 限 IOPS（混部）: echo '8:0 rbps=104857600 wiops=2000' > /sys/fs/cgroup/mixed/io.max"
    ;;
*)
    echo "用法: $0 bench [设备] | ionice <pid>"; exit 1;;
esac
