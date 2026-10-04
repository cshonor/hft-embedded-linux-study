#!/bin/bash
# fio-latency-vs-throughput.sh — iodepth 决定你测的是什么（ch12 §12.2 块3）
# 出自: notes/section-12.2-基准测试的类型.md
# 用法: ./fio-latency-vs-throughput.sh [设备]
set -u

DEV="${1:-/dev/nvme0n1}"
command -v fio >/dev/null || { echo "未安装: sudo apt install fio"; exit 1; }

echo "===== ① iodepth=1：单请求往返延迟（4k 随机写，只读安全改用 randread） ====="
fio --name=latency --filename="$DEV" --direct=1 \
    --rw=randread --bs=4k --iodepth=1 --runtime=60 --time_based \
    --group_reporting --percentile_list=50:99:99.9 --readonly

echo
echo "===== ② iodepth=32：排队吞吐（同一个设备，回答的是另一个问题） ====="
fio --name=throughput --filename="$DEV" --direct=1 \
    --rw=randread --bs=4k --iodepth=32 --runtime=60 --time_based \
    --group_reporting --readonly

cat <<'EOF'

对照读法：
  QD1  的延迟 ≈ 设备真实服务水平（HFT 日志盘关心这个）
  QD32 的 IOPS ≈ 设备吞吐上限（容量规划关心这个）
  Little's Law  sanity check: IOPS ≈ QD / latency
  （QD32 的"单请求延迟"必然 ≈ 32 × QD1 延迟，不是设备变慢了）
EOF
