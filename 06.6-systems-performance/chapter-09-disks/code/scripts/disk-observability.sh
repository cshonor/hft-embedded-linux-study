#!/bin/bash
# disk-observability.sh — 块 I/O 观测：biolatency/biosnoop/biostacks + 两条 bpftrace
# 出自: notes/section-9.6-观测工具.md 块4/5
# 全部需要 root（BPF 加载）
set -u

echo "===== ① biolatency：分类型延迟直方图（5ms 粒度） ====="
echo "sudo biolatency-bpfcc -F -m 5"

echo
echo "===== ② biosnoop：逐次 I/O（谁、多大、多慢） ====="
echo "sudo biosnoop-bpfcc"
echo "# TIME(s) COMM PID DISK T SECTOR BYTES LAT(ms)"

echo
echo "===== ③ biostacks：带内核栈（慢 I/O 是谁触发的调用链） ====="
echo "sudo biostacks-bpfcc   # 本机若无比命令，用 biosnoop 替代"

echo
echo "===== ④ bpftrace 两条 ====="
echo "# 按进程直方图化块 I/O 大小："
echo "sudo bpftrace -e 'tracepoint:block:block_rq_issue { @bytes[comm] = hist(args->bytes); }'"
echo
echo "# 只抓 >10ms 的慢 I/O："
echo "sudo bpftrace -e 'tracepoint:block:block_rq_complete /args->delta > 10000000/ { printf(\"%s %d\\n\", comm, args->delta/1000); }'"
