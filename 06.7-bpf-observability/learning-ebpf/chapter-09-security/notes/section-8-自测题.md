# 自测题

1. 安全工具与可观测工具的本质区别是什么？策略为什么要考虑错误路径？
2. seccomp 严格模式允许哪四个 syscall？seccomp-bpf 的四种输出动作？
3. seccomp-bpf 的两大局限是什么？
4. eBPF 生成 seccomp profile 挂在哪个 tracepoint？为什么说每个进程加载独立程序是常见模式？
5. 画出 TOCTOU 竞态窗口。为什么 seccomp-bpf 反而不受影响？Sysmon for Linux 如何缓解、代价是什么？
6. BPF LSM 为什么天然免疫 TOCTOU？返回值语义是什么？需要哪个内核版本？
7. Tetragon 为什么选 `fd_install` 而不是 `open` syscall？
8. `bpf_send_signal` 实现的防护与"通知用户态再处理"有何本质不同？
9. 为什么网络安全工具普遍用防护模式而主机侧长期用审计模式？
