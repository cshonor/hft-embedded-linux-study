#!/bin/bash
# bpftool-cheatsheet.sh — bpftool 六连（02-ch02 §3 块5）
# 出自: notes/section-3-扩展版BPF.md
set -u

echo "===== ① 已加载程序 ====="
bpftool prog show 2>/dev/null | head -10 || sudo bpftool prog show | head -10

cat <<'EOF'

===== ② 反汇编（三种视角） =====
bpftool prog dump xlated id <N>          # BPF 指令（含 C 源码行，若有 BTF+linum）
bpftool prog dump xlated id <N> linum    # 带源码行号
bpftool prog dump jited id <N>           # 宿主机 JIT 后的真实机器码

===== ③ map =====
bpftool map show
bpftool map dump id <N>                  # 内容

===== ④ BTF =====
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h   # 内核 BTF → C 头（CO-RE 用）
bpftool btf dump prog id <N>              # 单个程序的 BTF
EOF
