#!/bin/bash
# fs-oneliners.sh — 文件系统域命令精选（02-ch08 §4/9 块5-7）
# 全部需要 root 执行
set -u

cat <<'EOF'
===== VFS 统计（§4 块5） =====
funccount-bpfcc 'vfs*'
bpftrace -e 'kprobe:vfs* { @[func] = count(); }'

===== BCC 单行（§9 块6） =====
trace-bpfcc 't:syscalls:sys_enter_creat' '%s', args->pathname
argdist-bpfcc -e 't:syscalls:sys_enter_newstat():char*:args->filename'
funccount-bpfcc 't:syscalls:sys_enter_*read*'      # read 变体分布
argdist-bpfcc -c 't:syscalls:sys_exit_read():int:args->ret:args->ret<0'   # 按错误码
funccount-bpfcc 't:ext4:*'                          # ext4 tracepoint 覆盖
stackcount-bpfcc -u ext4_file_read_iter             # ext4 读的用户栈
stackcount-bpfcc -P ext4_readpages                  # 预读取落盘路径

===== bpftrace 单行（§9 块7） =====
bpftrace -e 't:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'
bpftrace -e 't:syscalls:sys_enter_newstat { @[str(args->filename)] = count(); }'
bpftrace -e 'tracepoint:syscalls:sys_enter_*write* { @[probe] = count(); }'
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @ = hist(args->ret); }'
bpftrace -e 'kprobe:ext4_file_read_iter { @[ustack, comm] = count(); }'
EOF
