#!/bin/sh
# config-snapshot.sh — 配置快照（ch16 §16.1.3-16.1.4 块2，部署钩子里调用）
# 用途: "变快/变慢"类案例的第一证据——部署前后系统配置到底改了什么
# 用法: 部署流水线里加一步  ./config-snapshot.sh [输出根目录]
SNAP="${1:-/var/lib/perf-snap}/$(date +%Y%m%d-%H%M%S)"
mkdir -p "$SNAP" 2>/dev/null || SNAP="/tmp/perf-snap-$(date +%Y%m%d-%H%M%S)" && mkdir -p "$SNAP"

sysctl -a                     > "$SNAP/sysctl.txt"      2>/dev/null
cat /proc/cmdline             > "$SNAP/cmdline.txt"
cat /proc/mounts              > "$SNAP/mounts.txt"
cat /proc/interrupts          > "$SNAP/interrupts.txt"
lscpu                         > "$SNAP/lscpu.txt"
cpupower frequency-info       > "$SNAP/cpufreq.txt"     2>/dev/null
dmesg                         > "$SNAP/dmesg.txt"       2>/dev/null
dpkg -l 2>/dev/null | awk '$1=="ii"{print $2,$3}' > "$SNAP/packages.txt"
uname -a                      > "$SNAP/uname.txt"

echo "快照完成: $SNAP"
echo "对比用法: diff $SNAP/sysctl.txt <上一次快照>/sysctl.txt"
