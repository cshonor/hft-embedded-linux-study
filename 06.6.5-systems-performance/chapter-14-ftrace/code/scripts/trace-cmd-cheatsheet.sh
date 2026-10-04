#!/bin/bash
# trace-cmd-cheatsheet.sh — trace-cmd / perf ftrace 前端速查（ch14 §14.11-14.13 块1/2/3）
# 比手写 tracefs 更安全（record 自己管 tracing_on 开关）。需要 root。
set -u
[ "$(id -u)" = 0 ] || { echo "需要 root"; exit 1; }

cat <<'EOF'
===== trace-cmd（推荐前端） =====
# 录制 sched 事件 5 秒
trace-cmd record -e sched sleep 5
trace-cmd report | head -40          # 解码回放

# 组合：函数图 + 双事件系统 + 指定函数
trace-cmd record -p function_graph -l tcp_recvmsg -e sched -e net sleep 10

# 归档：trace.dat 自包含（含事件 format、buffer、CPU 信息）
trace-cmd record -o case42.dat -e sched -e irq -e hwlat sleep 60

# 事后提取（机器崩前留在 buffer 里的内容）
trace-cmd extract -o crash.dat

===== perf ftrace（perf 系用户的前端） =====
perf ftrace --tracer function_graph -- sleep 5
perf ftrace -G __x64_sys_epoll_wait -- sleep 3    # -G 指定 graph 根
perf ftrace --tracer function -G tcp_v4_rcv -- sleep 3

选哪个：
  要归档/跨机分析 → trace-cmd（trace.dat 自包含）
  已在 perf 生态里  → perf ftrace
  要定制 hist/合成事件 → 手写 tracefs（见 ftrace-events.sh / ftrace-wakeup-latency.sh）
EOF
