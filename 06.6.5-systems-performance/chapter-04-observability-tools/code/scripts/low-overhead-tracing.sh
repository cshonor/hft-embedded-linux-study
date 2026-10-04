#!/bin/bash
# low-overhead-tracing.sh — 生产环境低开销追踪（别用 strace -p）
# 出自: notes/section-4.5-四大追踪器.md 块6
# 用法: ./low-overhead-tracing.sh <PID>
set -u

PID="${1:?用法: $0 <PID>}"

echo "# 错（生产禁用）：strace -p $PID   ← ptrace 停进程，开销 10-100×"
echo
echo "# 对①：perf trace（单点旁路，不 ptrace 停进程，仍有开销但低一个量级以上）"
perf trace -p "$PID" 2>&1 | head -30

echo
echo "# 对②：bpftrace 聚合统计（不出每事件明细，只出计数，开销最低）"
echo "bpftrace -e 'tracepoint:raw_syscalls:sys_enter @[comm] = count();'"
echo "（需要 root；按 Ctrl-C 后打印聚合表）"
