#!/bin/bash
# bcc-checklist.sh — BCC 工具检查清单（02-ch03 §4 块2）
# 出自《BPF Performance Tools》第 3 章：60 秒十条之后的"BPF 版第二遍"
# 全部需要 root（Debian/Ubuntu 上命令带 -bpfcc 后缀）
set -u

cat <<'EOF'
===== BCC 检查清单（按问题域分组，全部 sudo） =====

-- 进程/执行 --
execsnoop-bpfcc        # 新进程逐次（execve 追踪，找短生命周期进程）
opensnoop-bpfcc        # open 逐次（找配置/日志文件访问）

-- 文件系统/块 I/O --
ext4slower-bpfcc 10    # ext4 慢操作 >10ms（换文件系统改 xfslower 等）
biolatency-bpfcc -m 5  # 块 I/O 延迟直方图
biosnoop-bpfcc         # 块 I/O 逐次（谁、多大、多慢）
cachestat-bpfcc 5      # 页缓存命中率

-- 网络 --
tcpconnect-bpfcc       # 出站连接逐次（谁在外联）
tcpaccept-bpfcc        # 入站连接逐次
tcpretrans-bpfcc       # 重传逐次（"变快/变慢"的另一种真相）

-- CPU/调度 --
runqlat-bpfcc 10       # 调度延迟直方图（等多久上 CPU）
profile-bpfcc 30       # 定频采样栈（on-CPU profiler）

===== 用法 =====
  先跑 linux-60s.sh 定位资源域，再从对应组挑工具下钻
  本机全部已装（bpfcc-tools 0.29.1）；没有 -bpfcc 后缀的发行版直接用原名
EOF
