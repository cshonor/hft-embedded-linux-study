#!/bin/bash
# app-oneliners.sh — 应用域 bpftrace 单行精选（02-ch13 §10 块1）
# 全部需要 root 执行
set -u

cat <<'EOF'
# 带参数的新创建进程
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'

# 49Hz 采样指定进程用户栈
bpftrace -e 'profile:hz:49 /comm == "mysqld"/ { @[ustack] = count(); }'

# 全栈三维采样
bpftrace -e 'profile:hz:49 { @[ustack, kstack, comm] = count(); }'

# malloc 字节数按用户栈聚合（高开销！每 malloc 一次探针）
bpftrace -e 'u:/lib/x86_64-linux-gnu/libc.so.6:malloc { @[ustack(5)] = sum(arg0); }'

# kill() 信号追踪：谁发的、发给谁、几号
bpftrace -e 't:syscalls:sys_enter_kill { printf("%s -> PID %d SIG %d\n", comm, args->pid, args->sig); }'

# pthread 条件变量函数计数 1 秒
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_cond_* { @[probe] = count(); } interval:s:1 { exit(); }'

# LLC cache miss 按进程（hardware 事件，需 PMU）
bpftrace -e 'hardware:cache-misses { @[comm] = count(); }'

===== MySQL USDT（§7 块6，需 mysqld 带 USDT） =====
usdt:/usr/sbin/mysqld:mysql:query__start { @query[tid] = str(arg0); @start[tid] = nsecs; }
usdt:/usr/sbin/mysqld:mysql:query__done  /@start[tid]/ {
    printf("%s %llu ms\n", str(@query[tid]), (nsecs - @start[tid]) / 1000000);
    delete(@query[tid]); delete(@start[tid]);
}
EOF
