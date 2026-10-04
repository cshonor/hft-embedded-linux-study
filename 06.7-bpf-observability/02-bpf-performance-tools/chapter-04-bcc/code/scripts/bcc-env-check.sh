#!/bin/bash
# bcc-env-check.sh — BCC 运行环境的内核配置八查（02-ch04 §3 块7）
# 出自: notes/section-3-BCC的安装.md
set -u

CFG=/boot/config-$(uname -r)
[ -f "$CFG" ] || CFG=/proc/config.gz

echo "内核: $(uname -r)，配置: $CFG"
echo

if [ "$CFG" = /proc/config.gz ]; then
    zgrep -E 'CONFIG_BPF=|CONFIG_BPF_SYSCALL|CONFIG_BPF_JIT|CONFIG_UPROBE|CONFIG_KPROBE|CONFIG_TRACING|CONFIG_FTRACE_SYSCALLS|CONFIG_DEBUG_INFO_BTF' "$CFG" 2>/dev/null
else
    grep -E 'CONFIG_BPF=|CONFIG_BPF_SYSCALL|CONFIG_BPF_JIT|CONFIG_UPROBE|CONFIG_KPROBE|CONFIG_TRACING|CONFIG_FTRACE_SYSCALLS|CONFIG_DEBUG_INFO_BTF' "$CFG"
fi

cat <<'EOF'

缺项后果速查：
  CONFIG_BPF/SYSCALL/JIT    → BPF 根本不可用（重编内核）
  CONFIG_KPROBE/UPROBE      → kprobe/uprobe 挂点全灭
  CONFIG_TRACING            → tracepoint 全灭
  CONFIG_FTRACE_SYSCALLS    → syscall tracepoint 没有
  CONFIG_DEBUG_INFO_BTF     → CO-RE 不可用（退回 BCC 现场编译）
EOF
