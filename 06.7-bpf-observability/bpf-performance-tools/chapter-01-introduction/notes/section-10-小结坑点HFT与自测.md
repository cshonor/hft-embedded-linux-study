# 1.10 小结 + 坑点 / HFT / 自测

> 底本：《BPF之巅》中文版 1.10 节（PDF p54–55）

## 本章小结

第 1 章是全书导论，建立六件事：

1. **BPF 定义**：内核/应用事件发生时运行小程序的机制——指令集 + 存储对象 + 辅助函数，解释器 + JIT 双执行，验证器保安全（不保逻辑）
2. **术语分层**：跟踪（事件记录）⊂ 可观测性（含采样与计数器工具，不含 benchmark）；嗅探 = 跟踪的别称
3. **前端格局**：BCC（70+ 工具、复杂参数、后台常驻）与 bpftrace（20+ 工具、单行/短脚本、定制问答）互补，同属 IOVisor 项目；ply 面向嵌入式
4. **两个上手工具**：execsnoop（逐事件、短命进程、业务负载画像）与 biolatency（内核态摘要、延迟直方图、双峰+离群点判读）
5. **插桩选型**：动态（kprobes/uprobes，任意函数、零启用成本、升级易破）vs 静态（tracepoint/USDT，稳定、防内联、数量少）——**先静态后动态**
6. **同一工具两副面孔**：opensnoop 的 bpftrace 版（改源码定制）与 BCC 版（-x/-p/-n 参数化）

## 坑点清单

| # | 坑 | 规避 |
|---|---|---|
| 1 | 只跟踪 open(2) 漏掉 openat(2)（308:5 的差距） | 跟踪系统调用家族先 `bpftrace -l '前缀*'` 列全部变体 |
| 2 | kprobe 目标被重命名/内联后脚本**静默无输出** | 检查 `Attaching N probes` 的 N；长期脚本优先 tracepoint |
| 3 | kretprobe 只挂一个 return 点漏计时 | 返回探针自动覆盖全部返回点，别手工模拟 |
| 4 | 只看延迟平均值错过双峰/离群点 | 用直方图类工具（biolatency/runqlat）看分布形态 |
| 5 | 用 ps/top 找短命进程 | 事件驱动（execsnoop）是唯一可靠方式 |
| 6 | 函数偏移量跟踪当稳定接口用 | 比函数入口更不稳定，只作临时手段 |

## HFT 关联

- **开市前体检**：`execsnoop` 跑 1 小时确认无非策略进程；`biolatency -m` 确认本地盘无 32ms+ 排队峰
- **交易路径分层选型**：1.5 节全景图 = 行情网卡→TCP→套接字→VFS→调度器逐层下钻的工具地图
- **零成本待命**：动态插桩未启用零开销——平时不挂探针，出事再挂，不引入常驻延迟
- **自家软件埋 USDT**：策略框架埋 `order__submit` 等稳定探针，版本迭代中观测脚本不变

## 自测题

<details>
<summary>📝 点击展开</summary>

1. **用一句话向同事解释 BPF 是什么，要求让他们立刻明白与 perf/strace 的区别。**

   <details><summary>参考答案</summary>

   "内核/应用事件发生时，内核里现场运行一段小程序来统计或记录——摘要零拷贝、逐事件不漏、观测窗口秒级即插即拔"；perf/strace 是固定功能工具，BPF 让你自己定义观测逻辑。

   </details>

2. **生产交易机上要长期挂一个统计 vfs_read 延迟的脚本，tracepoint 和 kprobe:vfs_read 选哪个？为什么？**

   <details><summary>参考答案</summary>

   先查有无对应 tracepoint（`bpftrace -l 'tracepoint:*vfs*'`）；有则用 tracepoint——内核小版本升级接口稳定；没有才用 kprobe，并接受升级后可能失效的风险（做好探针数量校验与告警）。

   </details>

3. **execsnoop -t 发现每秒批量 30 个进程创建，说明什么类型的问题？这属于哪个方法论？**

   <details><summary>参考答案</summary>

   有服务被反复拉起又失败（配置错误/crash loop）。属于业务负载画像（workload characterization）方法论：先定性"系统在忙什么"，往往无需下钻即可定位并解决。

   </details>

</details>

## 交叉引用

- 下一章：[Ch 2 技术背景](../chapter-02-technology-background/)——BPF 指令集、调用栈、火焰图、六大事件源深入
- BCC 专章：[Ch 4](../chapter-04-bcc/) · bpftrace 专章：[Ch 5](../chapter-05-bpftrace/)
- 方法论展开：[Ch 3 性能分析](../chapter-03-performance-analysis/)
- 单行程序全集：[附录 A](../appendix-A-bpftrace单行命令.md) · BCC 开发入门：[附录 C](../appendix-C-BCC工具开发.md)
- 现代开发栈（libbpf/CO-RE）对照：[learning-ebpf Ch5](../../learning-ebpf/chapter-05-core-btf-libbpf/)
