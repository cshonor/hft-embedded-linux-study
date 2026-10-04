#!/bin/bash
# bcc-toolbox.sh — BCC 瑞士军刀四件：funccount/stackcount/trace/argdist（02-ch04 §5-8 块8-13）
# 出自: notes/section-5-funccount.md ~ section-8-argdist.md
# 全部需要 root；Debian/Ubuntu 带 -bpfcc 后缀
set -u

cat <<'EOF'
===== ① funccount：函数被调频率（块8 五式） =====
funccount-bpfcc tcp_drop                  # 内核函数频率
funccount-bpfcc 'vfs_*'                   # 通配
funccount-bpfcc -i 1 pthread_mutex_lock   # 每秒打印（-i 改间隔）
funccount-bpfcc 'c:strlen'                # 库函数（libc）
funccount-bpfcc 't:syscalls:sys_enter_*'  # tracepoint：全部 syscall 入口

===== ② stackcount：谁在调（栈聚合，块9/10） =====
stackcount-bpfcc ktime_get
stackcount-bpfcc -f ktime_get > out.folded && flamegraph.pl out.folded > ktime.svg
  # -f folded 格式 → 直接喂火焰图

===== ③ trace：逐次事件带参数（块11/12） =====
trace-bpfcc 'r::vfs_read (retval > 0) "read %d bytes", retval'   # 返回值过滤
trace-bpfcc -I 'net/sock.h' 'tcp_reset(struct sock *sk) "sk->sk_prot->name", sk'
  # -I 包含头文件后能用结构体字段；%K 把地址渲染成函数名
trace-bpfcc 'sock_alloc "%d", 1' 'r::sock_release "%d", -1'
  # 多探针组合：分配 +1 / 释放 -1 → 泄漏对账

===== ④ argdist：参数分布统计（块13） =====
argdist-bpfcc -H 'r::__tcp_select_window (retval > 0) "ret", retval'   # 直方图
argdist-bpfcc -C 't:tcp:tcp_probe:window'                              # 计数

选型：频率→funccount，谁调的→stackcount，逐次要参数→trace，参数分布→argdist
EOF
