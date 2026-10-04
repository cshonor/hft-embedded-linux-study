#!/bin/bash
# check-btf.sh — 内核 BTF 三查（ch05 §5.2 块1/2）
# 出自: notes/5.2_BTF与vmlinux.md
set -u

echo "===== ① 运行时接口（最常用）：/sys 暴露的 vmlinux BTF ====="
if ls -lh /sys/kernel/btf/vmlinux 2>/dev/null; then
    echo "→ 有 BTF，CO-RE 可用"
else
    echo "→ 不存在！CO-RE 不可用（只能 BCC 现场编译）"
fi

echo
echo "===== ② 内核配置确认 ====="
if zgrep CONFIG_DEBUG_INFO_BTF /proc/config.gz 2>/dev/null; then
    :
elif grep CONFIG_DEBUG_INFO_BTF /boot/config-$(uname -r) 2>/dev/null; then
    :
else
    echo "（/proc/config.gz 与 /boot/config 都查不到；以 ① 的实测为准）"
fi

echo
echo "===== ③ 附加：tracepoint format 也是类型信息来源（块2） ====="
head -8 /sys/kernel/tracing/events/syscalls/sys_enter_openat/format 2>/dev/null || \
    sudo head -8 /sys/kernel/tracing/events/syscalls/sys_enter_openat/format 2>/dev/null || \
    echo "（需 root 读 tracefs）"

echo
echo "===== ④ BTF 规模速览 ====="
if [ -f /sys/kernel/btf/vmlinux ]; then
    bpftool btf dump file /sys/kernel/btf/vmlinux format raw 2>/dev/null | wc -l | \
        xargs -I{} echo "vmlinux BTF 类型条目: {} 行"
fi
