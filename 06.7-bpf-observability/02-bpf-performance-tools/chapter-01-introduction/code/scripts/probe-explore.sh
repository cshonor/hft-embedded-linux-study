#!/bin/bash
# probe-explore.sh — 静态插桩探针勘探（02-ch01 §7 块1 + §8 块2/3/4）
# 出自: notes/section-7-静态插桩tracepoint与USDT.md、section-8-初识bpftrace跟踪open.md
set -u

echo "===== ① tracepoint 勘探 ====="
bpftrace -l 'tracepoint:syscalls:*open*' 2>/dev/null || sudo bpftrace -l 'tracepoint:syscalls:*open*'

echo
echo "===== ② 字段定义（args-> 后面能写什么就看这个） ====="
head -10 /sys/kernel/tracing/events/syscalls/sys_enter_openat/format 2>/dev/null || \
    sudo head -10 /sys/kernel/tracing/events/syscalls/sys_enter_openat/format

echo
echo "===== ③ USDT 探针（应用内嵌的静态探针，需 debug info） ====="
echo "bpftrace -l 'usdt:/usr/sbin/mysqld'   # 换成你的二进制"

cat <<'EOF'

===== ④ 书上三组对照实验（需 root 执行） =====
# 逐条打印（fprintf 是旧版写法，现用 printf）：
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'

# 通配挂点 + 按挂点计数：
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_open* { @[probe] = count(); }'
# 预期: openat 远多于 open（glibc 的 open() 内部走 openat）

# BCC 完整版: sudo python3 ../trace-open.py
EOF
