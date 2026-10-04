#!/bin/bash
# mem-triage.sh — 内存排查六连（ch07 §7.1/7.4）
# 出自: notes/section-7.1-7.2 块1、section-7.4-分析方法论.md 块3/4
set -u

echo "===== ① free -h —— 看 available，不看 free ====="
free -h
echo "（free 低≠没内存：buff/cache 可回收；available 才是可用量）"

echo
echo "===== ② vmstat —— si/so 持续非 0 要立刻查 ====="
vmstat 1 5
echo "（si=swap in, so=swap out；HFT 机器应恒为 0）"

echo
echo "===== ③ PSI memory —— 内存 stall ====="
cat /proc/pressure/memory
echo "（some avg10>10 = 有进程在等内存回收；full>0 = 全部都在等，严重）"

echo
echo "===== ④ slabtop —— 内核 slab 占用 top ====="
slabtop -o 2>/dev/null | head -15 || sudo slabtop -o | head -15

echo
echo "===== ⑤ OOM 历史 ====="
dmesg -T 2>/dev/null | grep -i oom | tail -5 || sudo dmesg -T | grep -i oom | tail -5 || echo "（无 OOM 记录）"

echo
echo "===== ⑥ NUMA 均衡 ====="
numastat -M 2>/dev/null | head -10 || numastat | head -10
