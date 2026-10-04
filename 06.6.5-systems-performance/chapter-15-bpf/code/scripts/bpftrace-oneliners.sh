#!/bin/bash
# bpftrace-oneliners.sh — bpftrace 单行集 + 探针勘探（ch15 §15.2 块4/5/7）
# 全部需要 root 执行；本脚本只做展示与 -l 勘探（-l 不需要 root 部分可用）
set -u

echo "===== ⓪ 探针勘探（先列再挂，避免拼写错误白跑） ====="
echo "bpftrace -l 'tracepoint:syscalls:*' | head"
bpftrace -l 'tracepoint:syscalls:*' 2>/dev/null | head -5 || echo "（-l 部分探针需 root）"
echo "bpftrace -l 'kprobe:*mutex*' | head"

cat <<'EOF'

===== ① syscall 计数（块4） =====
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_read { @ = count(); }'
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat /pid == 12345/ { @[comm] = count(); }'
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_{read,write} /pid==12345/ { @[probe] = count(); }'

===== ② 行为观察（块7） =====
# 新进程追踪（execve 逐次）
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'

# 每秒心跳
sudo bpftrace -e 'interval:s:1 { printf("tick\n"); }'

# 按进程统计块 I/O 大小分布
sudo bpftrace -e 'tracepoint:block:block_rq_issue { @bytes[comm] = hist(args->bytes); }'

# 内核函数调用频率 top
sudo bpftrace -e 'kprobe:vfs_* { @[func] = count(); }'   # Ctrl-C 后看排行

# 定频采样内核栈（自制 profiler 雏形）
sudo bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'

===== ③ uprobe 测函数延迟（块1/6 → 已落成脚本） =====
sudo bpftrace ../decode-latency.bt /opt/strategy decode
EOF
