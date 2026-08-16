# 自测题

1. trace_pipe 的三个局限是什么？生产环境用什么替代？
2. 加载 tracing 程序和 networking 程序分别需要哪些 capability？
3. `bpf_get_current_pid_tgid()` 和 `bpf_get_current_uid_gid()` 的 64 位返回值各字段怎么分布？
4. maps 的三大用途？per-CPU 变体解决什么问题？
5. 尾调用和普通函数调用的本质区别？为什么 eBPF 栈只有 512 字节使得尾调用更重要？
6. 33 层尾调用 × 100 万指令限制，对写复杂内核逻辑意味着什么？
