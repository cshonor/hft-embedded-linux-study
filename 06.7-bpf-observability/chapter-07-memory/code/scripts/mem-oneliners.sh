#!/bin/bash
# mem-oneliners.sh — 内存域命令精选（02-ch07 §4/5/6/10）
# 全部需要 root 执行
set -u

cat <<'EOF'
===== memleak（§4 块6，未释放分配对账） =====
memleak-bpfcc -p <PID>     # 用户态：malloc/calloc/realloc/free 对账
memleak-bpfcc              # 不带 -p：内核分配（kmem_cache_alloc 等）
# 输出: "bytes in N allocations from stack" = 仍未释放的分配栈

===== brk 观测（§5 块8） =====
trace-bpfcc -U t:syscalls:sys_enter_brk           # 逐事件
stackcount-bpfcc -P -u t:syscalls:sys_enter_brk   # 按进程聚合栈

===== 缺页观测（§6 块9） =====
stackcount-bpfcc -u t:exceptions:page_fault_user    # 用户态缺页 + 用户栈
stackcount-bpfcc    t:exceptions:page_fault_kernel  # 内核态缺页
stackcount-bpfcc -P -u t:exceptions:page_fault_user # 按进程

===== 内存规整耗时（§10 块2 模板） =====
tracepoint:compaction:mm_compaction_begin { @start[tid] = nsecs; }
tracepoint:compaction:mm_compaction_end /@start[tid]/ {
    @ns = hist(nsecs - @start[tid]);   // 配对三件套：存→过滤求差→delete
    delete(@start[tid]);
}
# compaction 频繁且耗时长 = THP 在后台搬页 → HFT 场景关 THP 的证据
EOF
