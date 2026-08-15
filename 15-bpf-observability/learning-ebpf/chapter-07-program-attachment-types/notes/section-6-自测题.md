# 自测题

1. 程序类型决定了哪四件事？
2. `BPF_KPROBE_SYSCALL` 与 `BPF_KPROBE` 与 `BPF_PROG` 各用于什么场景？
3. fexit 相比 kretprobe 的独有优势是什么？依赖哪个内核机制、哪个版本引入（x86/ARM 各是）？
4. tracepoint 的上下文结构怎么写？tp / raw_tp / tp_btf 三者的性能与可移植性权衡？
5. SOCKET_FILTER 过滤的是什么数据？为什么说它"名不副实"？
6. LSM 程序的返回码语义与追踪类有何不同？
7. 为什么一个接口挂第二个 XDP 程序会失败？多逻辑怎么组织？
8. helper 函数和 kfunc 在兼容性承诺上的区别？
