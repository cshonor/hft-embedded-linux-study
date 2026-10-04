#!/bin/bash
# unexplained-win-drill.sh — "莫名变快"案例演练全流程（ch16 §16.1 + §16.9 块1-9）
# 出自: notes/section-16.1.3-16.1.8 与 section-16.9-HFT-版Unexplained-Win演练模板.md
# 用法: ./unexplained-win-drill.sh <进程名>
# 思想: 先排分母（tick 速率变了？），再排配置（快照 diff），最后才上 perf——顺序见注释
set -u

PROC="${1:?用法: $0 <进程名>}"
PID=$(pidof "$PROC" | awk '{print $1}')
[ -n "$PID" ] || { echo "进程未找到: $PROC"; exit 1; }

echo "===== S1 分母核查：上游 tick 速率前后对比 ====="
echo "应用 counter / Prometheus：变快是不是因为活变少了？"
echo "打点: git diff <旧版本>..<新版本> -- src/telemetry/ src/latency_probe/"

echo
echo "===== S2 统计数据对比（案发日 vs 上周同 weekday 同时段） ====="
echo "sar -u -f /var/log/sa/sa30    # 案发日 CPU"
echo "sar -u -f /var/log/sa/sa23    # 上周同日"
echo "sar -q -f /var/log/sa/sa30    # load/runq"
mpstat -P ALL 1 3
vmstat 1 3

echo
echo "===== S3 配置事实核查（不是配置声明，是事实） ====="
diff /var/lib/perf-snap/*/sysctl.txt 2>/dev/null | head -10 || echo "（无历史快照，先用 config-snapshot.sh 建档）"
echo "THP: $(cat /sys/kernel/mm/transparent_hugepage/enabled)"
taskset -pc "$PID"
grep Cpus_allowed /proc/"$PID"/status
echo "governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)"

echo
echo "===== S4 perf stat 九事件（PMC + 软件事件） ====="
echo "perf stat -e cycles,instructions,cache-references,cache-misses,\\"
echo "  branch-instructions,branch-misses,context-switches,cpu-migrations,page-faults \\"
echo "  -p $PID -- sleep 10"

echo
echo "===== S5 on-CPU vs off-CPU 分野 ====="
echo "perf record -F 49 -g -p $PID -- sleep 10    # 采样率压低减扰动"
echo "perf archive new.perf.data                  # 存档供日后 diff"
echo "perf diff baseline.perf.data new.perf.data  # 逐符号 +xx%/-xx%"
echo "sudo runqlat-bpfcc 10                       # 调度延迟：等多久上 CPU"
echo "sudo offcputime-bpfcc -p $PID 10            # off-CPU 阻塞栈：等在哪"
echo "sudo tcpretrans-bpfcc                       # 重传消失=变快的另一种真相"
