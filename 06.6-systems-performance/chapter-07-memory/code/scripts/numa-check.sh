#!/bin/bash
# numa-check.sh — NUMA 拓扑检查与进程绑定（ch07 §7.3/7.6）
# 出自: notes/section-7.3-硬件与软件架构.md 块2、section-7.6 块7
# 用法: ./numa-check.sh [程序路径]   # 给程序则绑 node0 跑
set -u

echo "===== ① numactl --hardware —— 节点与距离 ====="
numactl --hardware

echo
echo "===== ② numastat —— 本地 vs 远程分配 ====="
numastat
echo "（remote 占比高 = 跨节点访问，每次 +40~80ns，HFT 不可接受）"

if [ $# -ge 1 ]; then
    echo
    echo "===== ③ 绑到 node 0 的 CPU + 内存运行: $* ====="
    numactl --cpunodebind=0 --membind=0 --preferred=0 "$@"
else
    cat <<'EOF'

===== ③ 绑定用法 =====
  numactl --cpunodebind=0 --membind=0 ./strategy
  # 配套：收包网卡的 PCIe 所在节点 = 绑定节点（lspci -vv 查 NUMA node）
EOF
fi
