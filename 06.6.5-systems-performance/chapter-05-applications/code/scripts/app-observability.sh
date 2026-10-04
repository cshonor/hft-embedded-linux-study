#!/bin/bash
# app-observability.sh — 应用级观测四件套（ch05 §5.5）
# 出自: notes/section-5.5-观测工具.md 块1/2/3
# 用法: ./app-observability.sh <进程名> [秒数]
# 覆盖: syscall 计数 / On-CPU（时间花在哪）/ Off-CPU（阻塞在哪）/ 线程级 pidstat
set -u

PROC="${1:?用法: $0 <进程名> [秒数(默认30)]}"
SECS="${2:-30}"
PID=$(pidof "$PROC" | awk '{print $1}')
[ -n "$PID" ] || { echo "进程未找到: $PROC"; exit 1; }
echo "目标: $PROC (pid=$PID)，采样 ${SECS}s"

echo
echo "===== ① syscall 计数（谁在用 syscall，多少次） ====="
echo "sudo syscount-bpfcc -p $PID $SECS   # 需 root，Ctrl-C 后出聚合表"

echo
echo "===== ② On-CPU：CPU 时间花在哪（剖析） ====="
echo "perf record -F 99 -g -p $PID -- sleep $SECS   # 99Hz 是安全默认值"
echo "# 或 BCC 版（内核+用户栈一次采全）: sudo profile-bpfcc -p $PID $SECS"

echo
echo "===== ③ Off-CPU：不在 CPU 上的时间花在哪（阻塞） ====="
echo "sudo offcputime-bpfcc -p $PID $SECS   # 等锁/等IO/被抢占 各多少"

echo
echo "===== ④ 线程级占用（TID 粒度，找异常线程） ====="
pidstat -t -p "$PID" 1 5
