#!/bin/bash
# bpftrace-oneliners-10.sh — bpftrace 单行精选（02-ch05 §5 块22、§1 块14、§14 块9-12）
# 全部需要 root 执行
set -u

cat <<'EOF'
===== 进程/文件行为（§5 块22） =====
# 1. 谁在执行什么命令
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'
# 2. execve 的 argv 逐个打印（join 按 8B 步长解引用 argv 数组）
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'
# 3. openat 打开的文件，按进程
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

===== syscall 聚合（§5 块22） =====
# 4. 按程序统计 syscall 次数
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }'
# 5. 按探针名看全部 syscall 分布
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[probe] = count(); }'
# 6. 按进程+PID 统计
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[comm, pid] = count(); }'
# 7. 按进程统计 read 总字节数
bpftrace -e 'tracepoint:syscalls:sys_exit_read /args->ret > 0/ { @[comm] = sum(args->ret); }'

===== 返回值直方图（§1 块14） =====
bpftrace -e 'kretprobe:vfs_read { @bytes = hist(retval); }'

===== map 操作（§14 块9-12） =====
# 通配 + probe 内置变量按探针计数
bpftrace -e 't:block:* { @[probe] = count(); }'
# 周期打印滚动计数
bpftrace -e 't:block:block_rq_insert { @ = count(); } interval:s:1 { print(@); clear(@); }'
# sum：read 总字节
bpftrace -e 't:syscalls:sys_exit_read /args->ret > 0/ { @bytes = sum(args->ret); }'
# lhist：线性直方图（read 返回值 0-1000 按 100 分桶）
bpftrace -e 't:syscalls:sys_exit_read { @ret = lhist(args->ret, 0, 1000, 100); }'
# END print top5
bpftrace -e 'kprobe:vfs* { @[probe] = count(); } END { print(@, 5); clear(@); }'
EOF
