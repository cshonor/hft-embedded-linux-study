#!/bin/bash
# disk-triage.sh — 磁盘排查五连（ch09 §9.5 块3）
# 出自: notes/section-9.5-分析方法论.md
set -u

echo "===== ① iostat -xz —— util/await/avgqu-sz ====="
iostat -xz 1 3
echo "（await 持续 >几 ms 且 avgqu-sz 涨 = 盘跟不上；util≈100% 虚拟盘有陷阱，见笔记）"

echo
echo "===== ② pidstat -d —— 哪个进程在读盘 ====="
pidstat -d 1 3

echo
echo "===== ③ PSI io —— I/O stall 证据 ====="
cat /proc/pressure/io

echo
echo "===== ④ dmesg —— 存储相关错误 ====="
dmesg 2>/dev/null | grep -iE "reset|error|fail|medium" | tail -10 || sudo dmesg | grep -iE "reset|error|fail|medium" | tail -10 || echo "（无存储错误记录）"

echo
echo "===== ⑤ swap 活动（盘忙的隐形原因） ====="
swapon --show
vmstat 1 3 | tail -4
