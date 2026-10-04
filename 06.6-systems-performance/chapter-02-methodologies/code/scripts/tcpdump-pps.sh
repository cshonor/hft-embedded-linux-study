#!/bin/bash
# tcpdump-pps.sh — tcpdump 交叉验证应用内 counter（P2 层）
# 出自: notes/section-2.5-性能分析方法论.md 块4
# 用法: ./tcpdump-pps.sh [网卡] [包数]
# 原理: pps ≈ (N-1) / (t_N - t_1)，与应用内 ticks/s 对不上就往 L0/L1 下钻
set -u

NIC="${1:-eth0}"
COUNT="${2:-1000}"
LOG="/tmp/ticks${COUNT}.log"

echo "抓 $COUNT 个包（$NIC）…（需要 root 或 CAP_NET_RAW）"
tcpdump -i "$NIC" -c "$COUNT" -tttt --no-promisc 2>/dev/null | tee "$LOG" >/dev/null

python3 - "$LOG" <<'EOF'
import sys, re
times = []
for line in open(sys.argv[1]):
    m = re.match(r'(\d+):(\d+):(\d+)\.(\d+)', line.strip().split()[-7] if ' ' in line else '')
    m = re.search(r'(\d{2}):(\d{2}):(\d{2})\.(\d{6})', line)
    if m:
        h, mnt, s, us = map(int, m.groups())
        times.append(h * 3600 + mnt * 60 + s + us / 1e6)
if len(times) >= 2:
    dt = times[-1] - times[0]
    print(f"首包 {times[0]:.6f}  末包 {times[-1]:.6f}  跨度 {dt:.3f}s")
    print(f"pps ≈ {len(times) - 1} / {dt:.3f} = {(len(times) - 1) / dt:.0f}")
    print("→ 与应用内 ticks/s counter 对比：对齐→拆 P99/reject；不对齐→ethtool/perf 下钻")
else:
    print("未解析到时间戳（没抓到包？检查网卡名/权限）")
EOF
