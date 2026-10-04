#!/bin/bash
# bcc-debug.sh — BCC 程序调试五连（02-ch04 §12 块2-6）
# 出自: notes/section-12-BCC的调试.md
set -u

cat <<'EOF'
===== ① 看 bpf_trace_printk 输出 =====
cat /sys/kernel/tracing/trace_pipe        # 或 bpftool prog tracelog
  （调试行写法: bpf_trace_printk("DBG req=%llx ts=%lld\n", req, ts);
   生产代码别留——trace_pipe 全局共享，别人也看得见）

===== ② 看工具生成的完整 BPF C =====
opensnoop-bpfcc --ebpf        # 输出整段 BPF C：结构、map、helper 全在眼前
  （学习 BCC 程序怎么写的最快路径：挑个成品工具 --ebpf 读一遍）

===== ③ 看谁占着 BPF 资源 =====
bpflist-bpfcc                 # PID COMM TYPE COUNT：哪个进程持有哪些 prog/map

===== ④ 加载失败看 dmesg =====
dmesg | tail
  # 典型: trace_kprobe: Could not insert probe at vfs_rread+0: -2
  #        （函数不存在/被内联 → 换 attach 点；vfs_rread 是笔误示范）

===== ⑤ verifier 日志 =====
bpftool prog load <prog.o> /dev/null -d   # -d 打 verifier 完整推理（见 01/ch06）
EOF
