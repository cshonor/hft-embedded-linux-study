#!/bin/bash
# ftrace-101.sh — tracefs 基础六步 + function/function_graph（ch14 §14.1-14.4 块5/6/7/8/9）
# 全部需要 root（写 tracefs）。先关总闸再读，防止边读边写。
set -u

TR=/sys/kernel/tracing
[ -d "$TR" ] || TR=/sys/kernel/debug/tracing
[ -d "$TR" ] || { echo "tracefs 未挂载: sudo mount -t tracefs nodev /sys/kernel/tracing"; exit 1; }
[ "$(id -u)" = 0 ] || { echo "需要 root（写 $TR）"; exit 1; }

echo "===== ① 看家底 ====="
cat $TR/available_tracers
echo "当前: $(cat $TR/current_tracer)（nop=空转）"

cat <<EOF

===== ② function tracer 最小流程（块6） =====
  echo function > $TR/current_tracer
  echo tcp_v4_rcv > $TR/set_ftrace_filter    # 只追这个函数（关键！不设=追全部）
  echo 1 > $TR/tracing_on
  # ... 产生负载 ...
  echo 0 > $TR/tracing_on                    # 先关再读
  cat $TR/trace | head -50

===== ③ filter 语法（块7） =====
  echo 'tcp_*' > $TR/set_ftrace_filter       # 覆盖写
  echo 'udp_rcv' >> $TR/set_ftrace_filter    # 追加
  echo > $TR/set_ftrace_filter               # 清空（= 追全部，危险）
  grep -c . $TR/available_filter_functions   # 通常 3~5 万个函数

===== ④ function_graph（块8/9） =====
  echo function_graph > $TR/current_tracer
  echo tcp_recvmsg > $TR/set_graph_function  # 只对以它为根的子树画图
  echo nofuncgraph-irq > $TR/trace_options        # 只看进程上下文
  echo nofuncgraph-sleep-time > $TR/trace_options # 排除睡眠，只看 CPU 时间
  echo 1 > $TR/tracing_on; sleep 2; echo 0 > $TR/tracing_on
  cat $TR/trace | head -80

===== ⑤ 读完清理 =====
  echo nop > $TR/current_tracer
  echo > $TR/set_ftrace_filter
  echo > $TR/set_graph_function
EOF
