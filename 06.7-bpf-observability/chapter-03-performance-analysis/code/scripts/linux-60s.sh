#!/bin/bash
# linux-60s.sh — Linux 60 秒分析十条（02-ch03 §3 块1）
# 出自《BPF Performance Tools》第 3 章（Netflix 著名的 first-60-seconds 清单）
# 用法: ./linux-60s.sh [采样秒数，默认 1]
set -u

I="${1:-1}"

echo "===== ① uptime（load 相对核数=$(nproc)） ====="; uptime
echo; echo "===== ② dmesg | tail（内核异常） ====="; dmesg 2>/dev/null | tail -5 || sudo dmesg | tail -5
echo; echo "===== ③ vmstat $I（r 队列、si/so、cs） ====="; vmstat $I 3
echo; echo "===== ④ mpstat -P ALL $I（per-CPU、%soft） ====="; mpstat -P ALL $I 3
echo; echo "===== ⑤ pidstat $I（谁在用 CPU） ====="; pidstat $I 3
echo; echo "===== ⑥ iostat -xz $I（util/await） ====="; iostat -xz $I 3
echo; echo "===== ⑦ free -m（available） ====="; free -m
echo; echo "===== ⑧ sar -n DEV $I（网卡吞吐） ====="; sar -n DEV $I 3
echo; echo "===== ⑨ sar -n TCP,ETCP $I（TCP 计数与错误） ====="; sar -n TCP,ETCP $I 3
echo; echo "===== ⑩ top（汇总） ====="; top -b -n 1 | head -20
