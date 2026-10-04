#!/bin/bash
# sar-cheatsheet.sh — sar 实时采样 + 历史回放速查
# 出自: notes/section-4.4-sar-工具.md 块4/5
# 用法: ./sar-cheatsheet.sh        # 实时采样（每项 1 秒 × 5 次）
#       ./sar-cheatsheet.sh 28      # 回放当月 28 号的历史数据
set -u

if [ $# -eq 0 ]; then
    echo "===== 实时采样（Ctrl-C 可提前停） ====="
    echo "--- CPU";            sar -u 1 5
    echo "--- 网络接口";      sar -n DEV 1 5
    echo "--- 运行队列/load"; sar -q 1 5
    echo "--- 内存";          sar -r 1 5
    echo "--- 分页统计";      sar -B 1 5
    echo "--- 块设备";        sar -d 1 5
    echo "--- swap 换入换出"; sar -W 1 5
else
    DAY="$1"
    F="/var/log/sysstat/sa${DAY}"
    echo "===== 历史回放（$F） ====="
    [ -f "$F" ] || { echo "无此文件（sysstat 未开启采集？看 /etc/default/sysstat）"; exit 1; }
    echo "--- 当天 CPU";      sar -u -f "$F"
    echo "--- 09:30-10:30";   sar -q -s 09:30:00 -e 10:30:00 -f "$F"
    echo "--- 当天网络";      sar -n DEV -f "$F"
fi
