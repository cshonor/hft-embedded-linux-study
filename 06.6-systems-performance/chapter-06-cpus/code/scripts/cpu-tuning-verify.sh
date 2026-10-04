#!/bin/bash
# cpu-tuning-verify.sh — CPU 调优生效验证四连（ch06 §6.9）
# 出自: notes/section-6.9-CPU-调优.md 块9/10
# 用法: ./cpu-tuning-verify.sh [热核列表，默认 2,3]
set -u

CORES="${1:-2,3}"

echo "===== ① isolcpus 生效：热核上无其他进程 ====="
echo "内核 isolated 列表: $(cat /sys/devices/system/cpu/isolated 2>/dev/null || echo 无)"
for c in ${CORES//,/ }; do
    echo "--- 核 $c 上的线程（应只有你的策略线程）:"
    ps -eLo psr,pid,comm | awk -v c="$c" '$1==c'
done

echo
echo "===== ② nohz 生效：热核中断计数几乎不涨 ====="
echo "watch -d cat /proc/interrupts   # 交互观察；热核那列应基本不动"
grep -E "^\s*(0|1|2|3):" /proc/interrupts | head -4

echo
echo "===== ③ 调度延迟：runqlat 热核直方图应全部 < 数十 µs ====="
echo "sudo runqlat-bpfcc 10   # 需 root"

echo
echo "===== ④ 定频生效：所有核恒定最高频 ====="
grep MHz /proc/cpuinfo | sort | uniq -c
echo "调速器: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)"
echo "（performance + 所有核同频 = OK；powersave + 频率分散 = 未调）"

echo
echo "===== ⑤ RT 带宽限制（防 RT 任务饿死系统） ====="
echo "sched_rt_runtime_us = $(cat /proc/sys/kernel/sched_rt_runtime_us)（每 1s 周期 RT 最多占这么多 us）"
echo "sched_rt_period_us  = $(cat /proc/sys/kernel/sched_rt_period_us)"
echo "（HFT 热路径若用 SCHED_FIFO，常把 runtime 调成 -1 解除限流——但要自担饿死风险）"
