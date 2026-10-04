#!/bin/bash
# compile-bpf.sh — libbpf 风格 .bpf.c 编译与加载（ch03 配套）
# 用法: ./compile-bpf.sh           —— 编译全部 .bpf.c
#       ./compile-bpf.sh load lo   —— 把 hello-xdp 挂到指定网卡（需 root）
#       ./compile-bpf.sh unload lo —— 卸载（需 root）
set -eu

cd "$(dirname "$0")/.."

case "${1:-compile}" in
compile)
    for f in *.bpf.c; do
        o="${f%.c}.o"
        clang -O2 -g -target bpf -D__TARGET_ARCH_x86 -I/usr/include/x86_64-linux-gnu -c "$f" -o "$o"
        echo "编译: $f → $o"
    done
    echo
    echo "反汇编看 BPF 指令: llvm-objdump -d hello-bpf2bpf.bpf.o | less"
    echo "（llvm-objdump 在 llvm 包中: sudo apt install llvm；"
    echo " 能看到 hello 里有一条 call 指令指向 get_opcode —— 真·BPF-to-BPF 调用）"
    ;;
load)
    NIC="${2:-lo}"
    [ -f hello-xdp.bpf.o ] || { echo "先运行: $0 compile"; exit 1; }
    MODE=xdpgeneric
    [ "$NIC" != "lo" ] && MODE=xdp
    ip link set dev "$NIC" $MODE obj hello-xdp.bpf.o sec xdp
    echo "已挂载到 $NIC（$MODE）。看输出: cat /sys/kernel/tracing/trace_pipe"
    ;;
unload)
    NIC="${2:-lo}"
    MODE=xdpgeneric
    [ "$NIC" != "lo" ] && MODE=xdp
    ip link set dev "$NIC" $MODE off
    echo "已从 $NIC 卸载"
    ;;
esac
