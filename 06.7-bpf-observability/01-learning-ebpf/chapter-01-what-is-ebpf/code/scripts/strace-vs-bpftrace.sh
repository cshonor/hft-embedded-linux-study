#!/bin/bash
# strace-vs-bpftrace.sh — 同一件事的两种视角（ch01 §1.2 块1）
# strace -c：进程级 syscall 统计（ptrace，高开销）
# bpftrace：系统级 syscall 计数（eBPF，低开销、全机器）
set -u

echo "===== ① strace -c（ptrace 停进程，逐 syscall 拦截统计） ====="
strace -c echo "hello" 2>&1 | head -15

echo
echo "===== ② eBPF 视角看同样的事（全系统、按进程聚合，需 root） ====="
echo "sudo bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }'"
echo
echo "对比要点："
echo "  strace -c   → 只看一个进程、高开销（每 syscall 两次 ptrace 停）、但能看参数/返回值"
echo "  bpftrace    → 看全系统、低开销（tracepoint 静态插桩）、但只给聚合数据"
echo "  Ch15(06.6) 的中间态: perf trace（单进程 + tracepoint，开销居中）"
