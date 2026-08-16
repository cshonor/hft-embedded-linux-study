# 演进时间线（记住这几个节点）

| 年份 | 内核 | 事件 |
|------|------|------|
| 1993 | — | BPF 论文（伪机器 + 指令集） |
| 1997 | 2.1.75 | BPF 进入 Linux，用于 tcpdump 高效抓包 |
| 2012 | 3.5 | **seccomp-bpf**：BPF 程序决定是否允许用户态系统调用——BPF 第一次跳出"包过滤" |
| 2014 | 3.18 | **eBPF**：指令集为 64 位重写、引入 maps、`bpf()` 系统调用、helper 函数、**验证器**。官方生日 2014-09-26（验证器+bpf syscall+maps 补丁集被接受） |
| 2015 | — | eBPF 可挂 **kprobes**，追踪革命起点；网络栈开始加 hook |
| 2016 | — | eBPF 工具进生产（Netflix/Brendan Gregg "superpowers"）；**Cilium** 发布（容器环境整条 datapath 换 eBPF） |
| 2017 | — | Facebook 开源 **Katran**（L4 负载均衡）；此后 facebook.com 每个包都过 eBPF/XDP |
| 2018 | — | eBPF 成为内核独立子系统（维护者 Borkmann/Starovoitov，后加 Nakryiko）；**BTF** 引入（可移植性） |
| 2020 | — | **LSM BPF**：第三大用例（安全）确立 |

指令数上限从 4096 → 100 万条已验证指令；尾调用/函数调用让限制实际失效。
