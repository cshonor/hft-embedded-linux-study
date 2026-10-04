# 10. 可选练习（10.5 Exercises）

> 底本：《BPF之巅》第 10 章 网络，10.5 节（印刷 p529–530）

共 13 题，第 9–13 题标注"进阶，未解决"（作者留给读者深入研究）。

## 基础题（1–8）

1. **手工 delay 探测**：ping 的 RTT 与 ss -i 的 RTT 对比——ICMP vs TCP 内核估算差异。
2. **sofdsnoop 实验**：通过 unix socket 传递 fd（SCM_RIGHTS）并观测。
3. **socketio 改造**：加按 pid 聚合变体。
4. **socksize 变体**：把 hist 改为 stats()（count/avg/total）。
5. **superping 扩展**：增加发送间隔统计（IPG）。
6. **netsize 对比**：nic 与内核栈尺寸差异（GRO/TSO 前后）。
7. **nettxlat-dev**：按设备统计发送延迟，评估读不稳定结构体的代价。
8. **练习跟踪点替换**：把书中任一 kprobe 版工具改写为跟踪点版（对照 tcpconnect→tcpconnect-tp.bt 模式）。

## 进阶题（9–13，未解决）

9. **每事件开销测量**：量化 BPF 程序本身在 10 万 pps 下的 CPU 代价。
10. **sock 直接上下文**：研究 BTF/CO-RE 直读 sock 字段替代手动缓存。
11. **TCP 状态全迁移分析**：tcpstates 输出→完整状态机时间线重建。
12. **XDP 观测**：XDP 程序自身的性能分析（XDP 于自身路径）。
13. **完整网络栈 profiler**：写一个贯穿应用→驱动的统一延迟分解工具。

## HFT 关联

- 题 9/13 正是 HFT 延迟预算（latency budget）工程的核心：每层耗时分解到函数级，才能知道优化哪一微秒。
- 题 1 的 ICMP vs TCP RTT 差异提示：不要用 ping 评估交易链路——协议路径不同，结论会骗人。

## 参考骨架（自测后再看）

题 3（socketio 按 pid 聚合——改键即可）：

```awk
kprobe:sock_recvmsg { @rx[pid, comm] = count(); }
kprobe:sock_sendmsg { @tx[pid, comm] = count(); }
// 原工具按 comm 聚合，多线程进程下（Java/Go）同 comm 混在一起；
// 加 pid 维度即可拆线程——pcomm/comm 的取舍见 ch05 5.15
```

题 4（socksize 改 stats()——一函数换三视图）：

```awk
kretprobe:sock_recvmsg /retval > 0 && retval < 0x7fffffff/ {
    @rxbytes = stats(retval);     // count + average + total 一次出全
}
// stats() vs hist()：看总量/均值用 stats（吞吐画像），看分布形状用
// hist（多峰/长尾）——同一数据两种问题
```

题 8（kprobe → 跟踪点改写范式——以 tcpconnect 为例）：

```awk
// kprobe 版（挂 tcp_v4_connect，内核函数名漂移风险）
// 跟踪点版：状态迁移即事件，TCP_CLOSE→TCP_SYN_SENT 的瞬间就是"发起连接"
tracepoint:sock:inet_sock_set_state /args->newstate == 2/ {   // TCP_SYN_SENT=2
    // 此处取 sock 指针（args->skaddr）缓存五元组+上下文；
    // 注意：状态迁移发生在软中断上下文——pid 不可靠（10.1.4 错误一），
    // 需要 pid 就得在别处（进程上下文）以 sock 为键补记
}
// 迁移收益：免函数改名/内联问题；代价：失去进程上下文——这就是
// "kprobe 精确但脆弱、跟踪点稳定但语义粗"在网络章的具体形态
```

<details>
<summary>自测题</summary>

1. 为什么作者把 9–13 题标为"未解决"？
   <details><summary>答案</summary>这些题需要的研究深度超出"换个探针/改个聚合"的层次：每事件开销测量要隔离 BPF 执行环境、BTF 直读 sock 要 CO-RE 工程化、统一延迟分解要贯穿全栈的统一键体系——每道都是一个小型研究课题（其中 XDP 自观测、全栈 profiler 至今仍是活跃方向）。</details>

2. 题 13 的"统一延迟分解"对 HFT 意味着什么？
   <details><summary>答案</summary>把"收包→软中断→协议栈→唤醒→应用处理"每段耗时按函数级拆开并配同一个关联键——即**延迟预算表（latency budget）的现场测量手段**。有了它，"优化哪一微秒"从直觉问题变成算术问题；没有它，一切微优化都是盲调。</details>
</details>
