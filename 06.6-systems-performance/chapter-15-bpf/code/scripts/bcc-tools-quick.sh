#!/bin/bash
# bcc-tools-quick.sh — BCC 成品工具四连（ch15 §15.1 块2）
# 出自: notes/section-15.1-BCC-BPF-Compiler-Collection.md
# 全部需要 root。本机 bpfcc-tools 0.29.1 已装。
set -u

echo "===== ① runqlat：调度延迟分布（Ch6 金标准，10 秒） ====="
echo "sudo runqlat-bpfcc 10"

echo
echo "===== ② biolatency：块 I/O 延迟直方图（按 flag 分组，毫秒，5 秒一轮） ====="
echo "sudo biolatency-bpfcc -F -m 5 10"

echo
echo "===== ③ tcplife：TCP 连接生命周期（Ch10） ====="
echo "sudo tcplife-bpfcc"

echo
echo "===== ④ offcputime：Off-CPU 栈（进程离开 CPU 都在等什么，Ch5） ====="
PROC="${1:-}"
if [ -n "$PROC" ]; then
    PID=$(pidof "$PROC" | awk '{print $1}')
    echo "sudo offcputime-bpfcc -p $PID 30"
else
    echo "sudo offcputime-bpfcc -p \$(pidof strategy) 30"
fi

echo
echo "选型口诀：有成品用成品（-bpfcc），没有才 bpftrace 单行，再没有才 BCC Python（见 ../bcc_skeleton.py）"
