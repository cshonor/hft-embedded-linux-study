#!/bin/bash
# ftrace-events.sh — 事件源四件套：tracepoint/filter/trigger/hist + 动态探针（ch14 §14.5-14.10 块10-15）
# 全部需要 root
set -u

TR=/sys/kernel/tracing
[ -d "$TR" ] || { echo "tracefs 未挂载"; exit 1; }
[ "$(id -u)" = 0 ] || { echo "需要 root"; exit 1; }

echo "===== ① tracepoint：开 sched_switch 流式观察（块10） ====="
cat $TR/events/sched/sched_switch/format   # 先看字段定义（自动生成）
cat <<EOF
  echo 1 > $TR/events/sched/sched_switch/enable
  echo 1 > $TR/tracing_on
  cat $TR/trace_pipe        # 流式；快照用 cat $TR/trace

===== ② filter：只留关心的（块13） =====
  echo 'pid == 4242' > $TR/events/sched/sched_switch/filter
  echo 'prev_pid == 4242 || next_pid == 4242' > $TR/events/sched/sched_switch/filter
  echo 0 > $TR/events/sched/sched_switch/filter      # 清除

===== ③ trigger：命中即取栈（块14） =====
  echo 'stacktrace if next_pid == 4242' > $TR/events/sched/sched_switch/trigger
  # 清除: echo '!stacktrace if next_pid == 4242' > .../trigger

===== ④ hist：内核内直方图，不出原始事件（块15） =====
  echo 'hist:keys=next_pid' > $TR/events/sched/sched_switch/trigger
  cat $TR/events/sched/sched_switch/hist
  # 输出: next_pid / count 两列——开销远低于 trace_pipe 全量读

===== ⑤ kprobe/kretprobe（块11） =====
  echo 'p:myrecv tcp_v4_rcv skb=%ax:s64' >> $TR/kprobe_events
  echo 'r:myrecv_ret tcp_v4_rcv \$retval' >> $TR/kprobe_events
  echo 1 > $TR/events/kprobes/myrecv/enable
  # 用完: echo '-:myrecv' >> $TR/kprobe_events

===== ⑥ uprobe（块12） =====
  echo 'p:libfoo_free /usr/lib/libfoo.so:0x11230 size=%ax' >> $TR/uprobe_events
EOF
