#!/bin/bash
# perf-install-check.sh — perf 正确安装与验证（Debian/Ubuntu）
# 出自: notes/section-4.1-工具覆盖范围与危机工具.md 块1/2
set -u

echo "===== ① 安装（需要 sudo） ====="
echo "sudo apt install linux-tools-common linux-tools-$(uname -r) bpftrace"

echo
echo "===== ② 验证 perf 能出数 ====="
if perf stat -e cycles,instructions true 2>&1 | grep -qE "cycles|instructions"; then
    echo "perf 硬件计数器 OK"
else
    echo "⚠ 硬件计数器不可用（可能 perf_event_paranoid=$(cat /proc/sys/kernel/perf_event_paranoid) 过高或虚拟机无 PMU）"
    echo "  尝试软件事件: perf stat -e task-clock,page-faults true"
    perf stat -e task-clock,page-faults true 2>&1 | tail -5 || true
fi

echo
echo "===== ③ 数据源速查（块3） ====="
echo "perf stat -e cycles,instructions,cache-misses,branch-misses -p <PID>"
