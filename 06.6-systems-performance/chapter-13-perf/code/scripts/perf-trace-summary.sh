#!/bin/bash
# perf-trace-summary.sh — perf trace 汇总模式 + mmap 尖刺排查套路（ch13 §13.11 块6/7）
# 用法: ./perf-trace-summary.sh <进程名> [秒数]
set -u

PROC="${1:?用法: $0 <进程名> [秒数(默认10)]}"
SECS="${2:-10}"
PID=$(pidof "$PROC" | awk '{print $1}')
[ -n "$PID" ] || { echo "进程未找到: $PROC"; exit 1; }

echo "===== ① 汇总模式：syscall 直方统计（不开逐条打印，开销可接受） ====="
perf trace -s -p "$PID" -- sleep "$SECS" 2>&1 | head -30

cat <<'EOF'

===== ② 案例套路（块7）：tick 偶发 200µs 尖刺、CPU 图无热点 =====
  perf trace -s 发现 mmap/munmap 每 tick 几百次
    → munmap 触发 TLB shootdown（IPI 广播）→ 尖刺来源
    → 修法：预分配 arena / mmap cache（06-linux-mm ch03）

其他形态：
  perf trace -e open,read,write,mmap -- sleep 3   # 只跟这几类
  perf trace -p PID -- sleep 5                    # 逐条视图（strace 式，开发用）
EOF
