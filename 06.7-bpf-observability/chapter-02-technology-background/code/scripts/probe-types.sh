#!/bin/bash
# probe-types.sh — 四种挂点类型对比（02-ch02 §7/8/9 块6-13）
# 出自: notes/section-7-kprobes.md、section-8-uprobes.md、section-9-跟踪点tracepoints.md
set -u

cat <<'EOF'
===== 挂点类型速查 =====

① kprobe（内核函数边界，动态）
   bpftrace -e 'kprobe:vfs* { @[probe] = count(); }'
   BCC:  b.attach_kprobe(event="vfs_read", fn_name="do_read")
   优点：任何内核函数都能挂
   缺点：内核版本间函数名/内联会变（→ CO-RE/fentry 更稳）

② uprobe（用户态函数边界，动态）
   bpftrace -e 'uprobe:/lib/x86_64-linux-gnu/libc.so.6:gethost* { @[probe] = count(); }'
   BCC:  b.attach_uprobe(name="libc", sym="getaddrinfo", fn_name="do_entry")
         b.attach_uretprobe(name="libc", sym="getaddrinfo", fn_name="do_return")

③ tracepoint（内核静态插桩，稳定 ABI）
   bpftrace -e 'tracepoint:sched:sched_process_exec { printf("exec by %s\n", comm); }'
   字段查询: cat /sys/kernel/tracing/events/sched/sched_process_exec/format
   BCC 兼容写法（块12）:
     if BPF.tracepoint_exists("sock", "inet_sock_set_state"): 用 tracepoint
     else: 退回 kprobe（老内核没这个 tracepoint）

④ USDT（应用静态插桩，稳定，零开销闲置）
   见 ../usdt-tick.c（sys/sdt.h 的 DTRACE_PROBE）

选型：能用静态（tracepoint/USDT）就不用动态（kprobe/uprobe）——静态是承诺不变的 ABI
EOF
