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

<details>
<summary>自测题</summary>

1. 为什么作者把 9–13 题标为"未解决"？
2. 题 13 的"统一延迟分解"对 HFT 意味着什么？
</details>
