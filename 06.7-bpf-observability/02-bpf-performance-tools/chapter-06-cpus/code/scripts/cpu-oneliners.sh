#!/bin/bash
# cpu-oneliners.sh — CPU 域 bpftrace 单行精选（02-ch06 §10 块1）
# 全部需要 root 执行
set -u

cat <<'EOF'
===== 进程行为 =====
bpftrace -e 't:syscalls:sys_enter_execve { join(args->argv); }'
bpftrace -e 't:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'

===== syscall 聚合 =====
bpftrace -e 't:raw_syscalls:sys_enter { @[comm] = count(); }'
bpftrace -e 't:raw_syscalls:sys_enter { @[pid, comm] = count(); }'
# 按 syscall 名统计（sys_call_table 反查名字，kaddr 读内核全局）
bpftrace -e 't:raw_syscalls:sys_enter { @[sym(*(kaddr("sys_call_table") + args->id*8))] = count(); }'

===== profile 采样（自制 profiler 件） =====
bpftrace -e 'profile:hz:99 { @[comm] = count(); }'                        # 谁在跑
bpftrace -e 'profile:hz:49 /pid == 189/ { @[ustack] = count(); }'         # 指定 PID 用户栈
bpftrace -e 'profile:hz:49 { @[ustack, kstack, comm] = count(); }'        # 全栈三维
bpftrace -e 'profile:hz:99 { @cpu = lhist(cpu, 0, 256, 1); }'             # 负载均衡（各核分布）

===== 内核函数 =====
bpftrace -e 'kprobe:vfs_* { @[func] = count(); }'
bpftrace -e 'kprobe:x2apic_send_IPI* { @[probe, kstack(5)] = count(); }'  # IPI（TLB shootdown 来源）

===== 用户态 =====
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_create { printf("%s by %s (%d)\n", probe, comm, pid); }'

===== 自愿 vs 非自愿切换（§11 块3 练习思路） =====
sched_switch 里 prev_state==TASK_RUNNING → 主动让出（等 IO）
              否则                    → 被抢占（时间片到/高优唤醒）
完整实现形态见 ../runqlat.bt 的镜像结构
EOF
