#!/bin/bash
# debug-tips.sh — BPF 排错三招（02-ch18 §2/3/5 块1/4/5/6）
# 出自《BPF Performance Tools》第 18 章（全书收尾：怎么知道你的观测本身没说谎）
set -u

cat <<'EOF'
===== ① 事件对账法（块1）：已知负载 vs 工具计数 =====
dd if=/dev/zero of=test bs=1 count=230000     # 制造 23 万次已知的写
funccount-bpfcc -d 10 'ext4_*'                # 跟踪 ext4 全部函数
# 预期: ext4_write_checks ≈ 230000（与实际写次数对得上）
# 对不上 → 探针挂错函数/走了别的路径（buffered vs direct、变体差异）
# 这是验证"我的探针确实看到了这件事"的标准方法

===== ② 事件缺失排查（§3 块4/5）：tracepoint 没有就自己造 =====
# 现象: 期望的事件计数为 0
perf stat -e block:block_rq_insert,block:block_rq_requeue -a
#   requeue=0 → 这条路径根本没发生（不是探针问题）
# 没有现成 tracepoint？perf probe 造一个动态的：
perf probe vfs_read
perf stat -e probe:vfs_read -a        # 计数正常 → 路径在走，只是没 tracepoint
perf probe --del probe:vfs_read       # 用完删

===== ③ 反馈回路陷阱（§5 块6）：别把输出写到被监控的对象上 =====
# 反例: 监控 ext4 写，同时把结果写到 ext4 的文件 →
#   每次 printf 都触发新的写事件 → 事件风暴 + 数据污染
bpftrace -e 'k:ext4_file_write_iter { ... }' > /ext4fs/out.file   # ✗ 错
bpftrace -e 'k:ext4_file_write_iter { ... }' > /tmp/out.file      # ✓ 写 tmpfs
# 同理: 监控网络时别把结果发到网络（ssh 管道里跑 bpftrace 会自激）

===== ④ 被丢事件（§5）：ring buffer 满了 =====
# bpftrace/BCC 输出出现 "Possibly lost N samples" =
# 生产速度 > 消费速度 → 换更大 buffer、降探针频率、或改聚合（hist 替代逐条 printf）
EOF
