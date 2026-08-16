# 自测题

1. bpf() 三个参数各是什么？内核里的 eBPF 程序访问 map 走什么路径？
2. BPF 对象的引用计数有哪四种来源？追踪类与网络类程序的差异？
3. 挂 kprobe 需要哪三个 syscall（按序）？为什么挂 raw tracepoint 只需一条 bpf()？
4. PERF_EVENT_ARRAY 里的条目数由什么决定？ring buffer 为何没有这个问题？
5. ppoll 和 epoll 管理 fd 集合的本质区别？
6. `bpftool map dump name config` 在找 map 和读元素两个阶段各用什么命令迭代？
