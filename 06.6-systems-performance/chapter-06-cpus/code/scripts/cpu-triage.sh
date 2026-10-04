#!/bin/bash
# cpu-triage.sh — CPU 排查六连 + PSI（ch06 §6.5/6.6）
# 出自: notes/section-6.5-性能分析方法论.md 块3、section-6.6-6.7 块4/5
set -u

echo "===== ① uptime —— load 相对核数（核数=$(nproc)） ====="
uptime

echo
echo "===== ② mpstat -P ALL —— per-CPU 使用率 + %soft ====="
mpstat -P ALL 1 3

echo
echo "===== ③ vmstat —— r 队列、cs 上下文切换 ====="
vmstat 1 5

echo
echo "===== ④ pidstat -t —— 线程级归属 ====="
pidstat -t 1 3

echo
echo "===== ⑤ PSI —— 线程因 CPU 不足 stall 的时间占比 ====="
cat /proc/pressure/cpu
echo "（some avg10 持续 >10 = CPU 真的不够；>25 严重）"

echo
echo "===== ⑥ perf stat -a —— 全系统 IPC（需 perf 权限） ====="
perf stat -a -- sleep 5 2>&1 | grep -E "insn per cycle|GHz|cpus-migrated" || \
    echo "（跳过：perf_event_paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)，需 sudo 调低）"

echo
echo "===== ⑦ 调度延迟 runqlat（生产限时长 10s，需 root） ====="
echo "sudo runqlat-bpfcc 10   # 直方图尾部 >1ms 要紧张"
