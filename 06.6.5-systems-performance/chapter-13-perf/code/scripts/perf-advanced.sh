#!/bin/bash
# perf-advanced.sh — perf 进阶五式：sched/lock/c2c/mem/annotate/uprobe（ch13 §13.12/13.3/13.10）
# 出自: notes/section-13.12 块8/9/10/12/13、section-13.3-13.7 块14
# 用法: ./perf-advanced.sh <进程名>
set -u

PROC="${1:?用法: $0 <进程名>}"
PID=$(pidof "$PROC" | awk '{print $1}')
[ -n "$PID" ] || { echo "进程未找到: $PROC"; exit 1; }

echo "===== ① perf sched：调度延迟（线程被唤醒后等多久才上 CPU） ====="
echo "perf sched record -p $PID -- sleep 10 && perf sched latency"
echo "perf sched timehist   # 时间线视图"

echo
echo "===== ② perf lock：锁争用 ====="
echo "perf lock record -p $PID -- sleep 10 && perf lock report"
echo "perf lock report -F held   # 按持有时间"

echo
echo "===== ③ perf c2c：伪共享取证（HITM = 缓存行在核间乒乓） ====="
echo "perf c2c record -p $PID -- sleep 15 && perf c2c report"
echo "（配套 demo: ../false_sharing_demo.c）"

echo
echo "===== ④ perf mem：load 延迟按数据地址聚合 ====="
echo "perf mem record -p $PID -- sleep 10 && perf mem report"

echo
echo "===== ⑤ perf annotate：逐指令热点 ====="
echo "perf record -F 99 -e cycles -p $PID -- sleep 30"
echo "perf annotate decode_tick   # 该函数逐指令样本占比 + 源码对照"

echo
echo "===== ⑥ perf probe：uprobe 插用户态函数 ====="
echo "perf probe -x /path/$PROC 'decode_entry'      # 创建 uprobe 事件"
echo "perf record -e probe_$PROC:decode_entry -p $PID -- sleep 10"
echo "perf probe -d 'decode_entry'                  # 用完删"
