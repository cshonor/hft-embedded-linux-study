#!/bin/bash
# bpf-syscall-strace.sh — 用 strace 看 BCC 加载一个 BPF 程序的完整 syscall 序列
# 对应: notes/4.1_bpf总览与strace实例.md、4.2_对象生命周期与kprobe挂载.md、
#       4.3_perf与RingBuffer.md、4.4_遍历map的syscall序列.md
#
# 用法: ./bpf-syscall-strace.sh [目标python脚本]
# 默认目标: ../../chapter-02-hello-world/code/hello_counter.py（需 root 运行）
#
# 你将看到笔记里每一条 strace 行的真实版本：
#   bpf(BPF_MAP_CREATE)        建 map（hello_counter 的 counter_table）
#   bpf(BPF_PROG_LOAD)         编译后加载程序
#   perf_event_open({type=6})  kprobe 也是一种 perf PMU 事件！（4.2）
#   perf_event_open({...,PERF_COUNT_SW_BPF_OUTPUT})  ring buffer（4.3）
#   epoll_create1              perf_buffer_poll 的底座（4.3）
#   bpf(BPF_MAP_GET_NEXT_ID/KEY)  遍历 map（4.4，bpftool map list 走的就是这个）
set -u

TARGET="${1:-../../chapter-02-hello-world/code/hello_counter.py}"
cd "$(dirname "$0")/.." || exit 1

echo "目标: $TARGET"
echo "strace 跟 bpf/perf_event_open/epoll 三类 syscall…"
echo "（BCC 加载需要 root；没 root 会在 BPF_MAP_CREATE 处报 EPERM）"
echo

strace -f -e trace=bpf,perf_event_open,epoll_create1,epoll_ctl \
       timeout 5 python3 "$TARGET" 2>&1 | \
  grep -E "bpf\(|perf_event_open|epoll" | head -40

cat <<'EOF'

===== 对照阅读 =====
  每一行 bpf(BPF_XXX, ...) 都能在 4.1/4.4 的笔记里找到解释；
  perf_event_open(type=6) 对应 4.2「kprobe 也是 perf 事件」；
  想只看 id 遍历序列：bpftool map list 时 strace 它：
    strace -e bpf bpftool map list 2>&1 | grep GET_NEXT
EOF
