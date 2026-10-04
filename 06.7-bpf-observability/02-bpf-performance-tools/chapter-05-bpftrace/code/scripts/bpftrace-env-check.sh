#!/bin/bash
# bpftrace-env-check.sh — bpftrace 环境五连查（02-ch05 §3 块15/20）
set -u

echo "===== ① 内核版本（≥4.9，舒服线 5.10+） ====="
uname -r

echo
echo "===== ② 内核配置五项（期望计数 5） ====="
grep -cE "CONFIG_(BPF|BPF_SYSCALL|BPF_JIT|HAVE_EBPF_JIT|BPF_EVENTS)=y" /boot/config-$(uname -r) 2>/dev/null || \
grep -cE "CONFIG_(BPF|BPF_SYSCALL|BPF_JIT|HAVE_EBPF_JIT|BPF_EVENTS)=y" <(zcat /proc/config.gz 2>/dev/null)

echo
echo "===== ③ bpftrace 版本（决定语法上限） ====="
bpftrace --version

echo
echo "===== ④ 端到端冒烟（探针+验证器+输出全链路，需 root） ====="
echo "sudo bpftrace -e 'kprobe:do_nanosleep { printf(\"%s\n\", comm); }'"

echo
echo "===== ⑤ 探针可见性 + BTF 解析 ====="
bpftrace -l 'kprobe:vfs_*' 2>/dev/null | head -5 || echo "（部分探针需 root）"
