#!/bin/bash
# fs-triage.sh — 文件系统排查五连（ch08 §8.5 块2/3）
# 出自: notes/section-8.5-分析方法论.md
set -u

echo "===== ① 挂载选项（noatime 有没有） ====="
mount | grep -v -E "proc|sys|dev|cgroup|tmpfs|overlay" | head -10
echo "（data 盘没挂 noatime → 每次读都更新 atime = 白送一次写）"

echo
echo "===== ② free -m —— cache 占用（可回收部分） ====="
free -m

echo
echo "===== ③ sar -v —— dentry/inode 增长 ====="
sar -v 1 3

echo
echo "===== ④ cachestat —— 页缓存命中率（需 root，5 秒） ====="
echo "sudo cachestat-bpfcc 5"
echo "（命中率 <90% 且读量大 → 工作集超出内存或缓存被挤）"

echo
echo "===== ⑤ ext4slower —— >10ms 的慢操作（需 root） ====="
echo "sudo ext4slower-bpfcc 10"

cat <<'EOF'

===== 附：清空 page cache（仅测试环境！生产禁止——会引发 I/O 风暴） =====
  echo 3 | sudo tee /proc/sys/vm/drop_caches
  用途：benchmark 前保证冷缓存起点一致
EOF
