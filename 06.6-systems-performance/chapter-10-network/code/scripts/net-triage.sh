#!/bin/bash
# net-triage.sh — 网络丢包排查三连（ch10 §10.6 块2/3）
# 用法: ./net-triage.sh [网卡，默认 eth0]
set -u

NIC="${1:-eth0}"

echo "===== ① ethtool -S $NIC —— NIC 级丢包 ====="
ethtool -S "$NIC" 2>/dev/null | grep -iE 'drop|miss|no_buf|discard' | grep -v ": 0$" || echo "（无非零丢包计数）"
cat <<'EOF'
读数：
  rx_missed:                  NIC RX ring 满（软中断跟不上）→ UDP 丢包①
  rx_no_buffer/no_dma_resources: 缓冲分配失败
  tx_discards:                发送侧丢
EOF

echo
echo "===== ② softnet_stats —— backlog 溢出 ====="
echo "各 CPU 第 2 列（dropped）非零项:"
awk '$2 != 0 {print "  cpu" NR-1 ": dropped=" $2}' /proc/net/softnet_stats || true
awk '$2 == 0 {ok++} END {if (ok) print "  （" ok " 个 CPU dropped=0）"}' /proc/net/softnet_stats
echo "（非零 = netdev_max_backlog 溢出 → UDP 丢包②）"

echo
echo "===== ③ 抓包守则（§10.5 块1） ====="
echo "必须带 filter + 限包数 + 限时长："
echo "  timeout 10 tcpdump -i $NIC -c 10000 -w /tmp/net.pcap 'udp port 5001 and host 10.1.2.3'"
echo "更好：mirror 口/交换机端口镜像 → 离线机器分析"
