# ch04 code · bpf(2) syscall 观察实验

> 本章笔记的代码块为 bpf() 原型与 strace 序列展示（教学引用），
> 可落盘的实践是"用 strace 亲眼看一遍 BCC 加载过程"。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [bpf-syscall-strace.sh](./scripts/bpf-syscall-strace.sh) | 4.1-4.4 | `strace -f -e bpf,perf_event_open,epoll_create1` 观察 BCC 加载 hello_counter.py 的完整 syscall 序列（MAP_CREATE→PROG_LOAD→perf_event_open→epoll） | `bash -n` ✓；strace 本机可用（实跑 BCC 加载需 root，脚本内已注明降级行为） |

## 对照表（笔记 ↔ 你将看到的 syscall）

| 笔记 | syscall |
|------|---------|
| 4.1 加载总览 | `bpf(BPF_BTF_LOAD)` / `bpf(BPF_MAP_CREATE)` / `bpf(BPF_PROG_LOAD)` |
| 4.2 kprobe 挂载 | `perf_event_open({type=6,...})` —— kprobe 也是 perf PMU 事件 |
| 4.3 ring buffer | `perf_event_open({...,PERF_COUNT_SW_BPF_OUTPUT})` + `epoll_create1` |
| 4.4 遍历 map | `bpf(BPF_MAP_GET_NEXT_ID)` → `bpf(BPF_MAP_GET_NEXT_KEY)` |
