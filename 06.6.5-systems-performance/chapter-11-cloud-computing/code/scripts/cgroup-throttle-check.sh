#!/bin/bash
# cgroup-throttle-check.sh — 容器 CPU 节流排查三件套（ch11 §11.3 块1/2）
# 出自: notes/section-11.3-操作系统虚拟化-容器.md
# 用法: ./cgroup-throttle-check.sh        # 看当前 cgroup
#       ./cgroup-throttle-check.sh watch  # 连续观察 nr_throttled 是否在涨
set -u

show() {
    echo "===== ① cpu.max —— quota 限制 ====="
    cat /sys/fs/cgroup/cpu.max 2>/dev/null || echo "（非 cgroup v2 或无限制）"
    echo "（形如 'max 100000' = 无限制；'50000 100000' = 限 50% CPU）"

    echo
    echo "===== ② cpu.stat —— 节流计数 ====="
    grep -E "nr_periods|nr_throttled|throttled_usec" /sys/fs/cgroup/cpu.stat 2>/dev/null
}

case "${1:-}" in
watch)
    echo "连续观察 nr_throttled（每秒一行，Ctrl-C 停）:"
    prev=0
    while true; do
        cur=$(awk '/nr_throttled/{print $2}' /sys/fs/cgroup/cpu.stat 2>/dev/null)
        [ "$cur" != "$prev" ] && echo "$(date +%T) nr_throttled: $prev → $cur  ⚠ 在涨 = 延迟尖刺的 cgroup 级证据"
        prev=$cur
        sleep 1
    done
    ;;
*)
    show
    cat <<'EOF'

读数：
  nr_throttled > 0 且在涨 = 延迟尖刺的 cgroup 级证据
  （应用层看是"偶发卡顿"，实际是 quota 周期末被节流）

宿主机上按 cgroup 追踪（需宿主权限）:
  bpftrace -e 'tracepoint:sched:sched_switch { @[cgroup] = count(); }'
EOF
    ;;
esac
