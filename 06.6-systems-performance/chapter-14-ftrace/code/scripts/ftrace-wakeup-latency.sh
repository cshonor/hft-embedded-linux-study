#!/bin/bash
# ftrace-wakeup-latency.sh — 合成事件测调度延迟直方图（ch14 §14.5-14.10 块16）
# 原理: wakeup 时按 pid 记时间戳 ts0；sched_switch 时求差 → 合成事件 wakeup_lat
# 需要 root。用完自动清理。
set -u

TR=/sys/kernel/tracing
[ -d "$TR" ] || { echo "tracefs 未挂载"; exit 1; }
[ "$(id -u)" = 0 ] || { echo "需要 root"; exit 1; }
cd "$TR/events/sched" || exit 1

echo "===== ① 定义合成事件 + 两条 hist trigger ====="
echo 'wakeup_lat u64 lat; pid_t pid' > $TR/synthetic_events
echo 'hist:keys=pid:ts0=common_timestamp.usecs' > sched_wakeup/trigger
echo 'hist:keys=next_pid:wakeup_lat=common_timestamp.usecs-$ts0:'\
'onmatch(sched.sched_wakeup).wakeup_lat($wakeup_lat,next_pid)' > sched_switch/trigger

echo "已开启，采 10 秒…（这期间产生点负载更好）"
sleep 10

echo
echo "===== ② 结果：wakeup 侧直方图（每个 pid 被唤醒次数与延迟） ====="
cat sched_wakeup/hist | head -30
echo
echo "（P50/P99 一目了然；比 runqlat-bpfcc 更底层、无 BPF 依赖）"

echo
echo "===== ③ 清理 ====="
echo '!hist:keys=next_pid:wakeup_lat=common_timestamp.usecs-$ts0:'\
'onmatch(sched.sched_wakeup).wakeup_lat($wakeup_lat,next_pid)' > sched_switch/trigger
echo '!hist:keys=pid:ts0=common_timestamp.usecs' > sched_wakeup/trigger
echo '!wakeup_lat u64 lat; pid_t pid' >> $TR/synthetic_events
echo "已清理"
