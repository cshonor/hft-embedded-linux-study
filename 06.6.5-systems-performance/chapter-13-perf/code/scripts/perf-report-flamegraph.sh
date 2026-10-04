#!/bin/bash
# perf-report-flamegraph.sh — report 三维分析 + 火焰图全管道（ch13 §13.10 块1/2/3）
# 前提: 已有 perf.data（perf record 产生）
# 用法: ./perf-report-flamegraph.sh [report|script|flame] [进程名]
set -u

MODE="${1:-report}"

case "$MODE" in
report)
    [ -f perf.data ] || { echo "无 perf.data，先 perf record"; exit 1; }
    echo "===== 纯文本热点表 ====="
    perf report --stdio --no-children | head -50
    echo
    echo "其他视角："
    echo "  perf report --sort comm,dso,symbol        # 进程/库/符号三维"
    echo "  perf report -g graph,0.5,caller           # 调用图（caller 视角）"
    echo "  perf report -g fractal,0.5,callee         # callee 视角（火焰图式）"
    ;;
script)
    [ -f perf.data ] || { echo "无 perf.data"; exit 1; }
    perf script | head -20
    echo "# 定制字段: perf script -F comm,pid,tid,cpu,time,ip,sym"
    ;;
flame)
    PROC="${2:?用法: $0 flame <进程名>}"
    command -v stackcollapse-perf.pl >/dev/null || {
        echo "需 FlameGraph: git clone https://github.com/brendangregg/FlameGraph && export PATH=\$PATH:\$PWD/FlameGraph"
        exit 1; }
    PID=$(pidof "$PROC" | awk '{print $1}')
    perf record -F 99 -g -p "$PID" -- sleep 60
    perf script | stackcollapse-perf.pl | flamegraph.pl --title="$PROC CPU" > "$PROC.svg"
    echo "已生成 $PROC.svg"
    ;;
esac
