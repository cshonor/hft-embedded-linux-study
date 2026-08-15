# 自测题

1. eBPF 编程的哪两半？bpftrace 如何把两者对程序员透明？
2. opensnoop 为什么必须同时挂 syscall 的 entry 和 exit？两个程序间如何传数据？
3. 为什么 Go/Java 不能作为内核侧 eBPF 语言？
4. BCC 的 `BPF_RINGBUF_OUTPUT` 一行为什么同时服务于内核和用户态？
5. libbpf 版 opensnoop 9MB vs Python 版 80MB，差的 71MB 是什么？
6. cilium/ebpf 的 bpf2go 生成哪些文件？为什么有两套字节码？
7. libbpfgo 与 cilium/ebpf 的架构差异是什么？各自的顾虑？
8. Aya 与 Redbpf、libbpf-rs 的本质区别？为什么 lockc 迁移到 Aya？
9. 如何查看一个已加载 eBPF 程序的运行次数与总耗时？
10. 多 eBPF 程序的应用中，程序之间如何协调？
