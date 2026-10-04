#!/bin/bash
# disk-queue-inspect.sh — 块设备队列参数解读（ch09 §9.4 块1/2）
# 用法: ./disk-queue-inspect.sh [设备名，默认 nvme0n1]
set -u

DEV="${1:-nvme0n1}"
Q="/sys/block/$DEV/queue"
[ -d "$Q" ] || { echo "无此设备: $DEV（ls /sys/block/ 看可用）"; exit 1; }

echo "===== $DEV 调度器 ====="
cat "$Q/scheduler"
echo "（NVMe 低延迟场景常见 [none]；mq-deadline 适合混合负载）"

echo
echo "===== $DEV 关键队列参数 ====="
for p in nr_requests read_ahead_kb rotational rq_affinity io_poll iopoll; do
    [ -e "$Q/$p" ] && printf "%-15s = %s\n" "$p" "$(cat "$Q/$p")"
done

cat <<'EOF'

解读：
  nr_requests   软件队列深度（排队上限）
  read_ahead_kb 预读窗口（顺序读放大；随机负载可调小）
  rotational    0=SSD（影响调度器默认行为）
  rq_affinity   完成中断的 CPU 亲和（1=同核组，低延迟关注）
  io_poll/iopoll 轮询模式（irq → poll，降延迟提 CPU 占用）
EOF
