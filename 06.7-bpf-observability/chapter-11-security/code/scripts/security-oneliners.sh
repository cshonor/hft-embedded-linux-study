#!/bin/bash
# security-oneliners.sh — 安全域命令精选（02-ch11 §2/8）
set -u

echo "===== ① BPF 安全相关 sysctl（§2 块4） ====="
sysctl -a 2>/dev/null | grep -E "bpf|jit" | head -8

cat <<'EOF'

读数：
  kernel.unprivileged_bpf_disabled = 1   # 非特权 BPF 已禁（默认安全姿态）
  net.core.bpf_jit_harden    = 0         # 生产建议 1（常数盲化防 JIT 喷洒）
  net.core.bpf_jit_kallsyms  = 0         # 1 = 暴露 JIT 符号给 root（调试用）

===== ② LSM 钩子审计（§8 块6-1） =====
funccount-bpfcc -p 1234 'security_*'
bpftrace -e 'kprobe:security_* /pid == 1234/ { @[probe] = count(); }'

===== ③ PAM 会话开始（§8 块6-2） =====
trace-bpfcc 'pam:pam_start "%s: %s", arg1, arg2'
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpam.so.0:pam_start { printf("%s: %s\n", str(arg0), str(arg1)); }'

===== ④ 内核模块加载（§8 块6-3） =====
trace-bpfcc 't:module:module_load "load: %s", args->name'
bpftrace -e 't:module:module_load { printf("load: %s\n", str(args->name)); }'

===== ⑤ RENAME_EXCHANGE 与进程树取证（已落成脚本） =====
sudo bpftrace ../rename-exchange-watch.bt        # 观测版
sudo bpftrace --unsafe ../rename-exchange-watch.bt kill   # 管控版（杀进程）
sudo bpftrace ../session-forensics.bt            # 会话取证（授权系统专用）
EOF
