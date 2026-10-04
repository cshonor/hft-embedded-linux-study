# 2. 分析策略（14.2 节）

> 底本：《BPF之巅》第 14 章 内核，14.2 节（印刷 p669–670）

内核性能分析的推荐九步策略（入门向，后文工具按此展开）：

1. **创建可触发相关事件的工作负载**，最好知道确定的触发次数——可能需要写一个简短的 C 程序（已知量验证法）
2. **检查现有跟踪点或工具**（包括本章介绍的工具）是否已对该事件插桩
3. 若事件**频繁调用且占 CPU >5%**：**CPU 剖析**快速查看涉及的内核函数（perf(1) 或 BCC profile(8) + CPU 火焰图）；不频繁的事件用长时间剖析积累样本。CPU 剖析还会展示**自旋锁的使用，以及乐观自旋期间的互斥锁**
4. 另一个找内核函数的方法：**对可能匹配的函数计数**——如分析 ext4 事件就对 `ext4*` 通配计数（BCC funccount(8)）
5. 对内核函数的**调用栈计数**了解代码路径（BCC stackcount(8)）；结果应与 CPU 剖析相符
6. 通过子事件**跟踪函数调用流**（perf-tools 中基于 Ftrace 的 **funcgraph(8)**）
7. 检查**函数参数**（BCC trace(8)、argdist(8)，或 bpftrace）
8. 测量**函数延迟**（BCC funclatency(8) 或 bpftrace）
9. **编写自定义工具**插桩事件，打印或汇总

> 可先用传统工具（14.3）走其中几步，再上 BPF 工具。

## HFT 关联

这套策略就是**内核新手的排障 SOP**，重点记忆三件套：funccount 摸频率（第4步）→ stackcount 看路径（第5步）→ funclatency 量延迟（第8步）。第 9 章的 nvmelatency 开发就是这套流程的完整示范（无跟踪点时 funccount 摸底 → 源码定边界 → 自定义工具）。

<details>
<summary>自测题</summary>

1. 第 3 步中 CPU 剖析能额外暴露哪两类锁行为？
   <details><summary>答</summary>自旋锁的使用，以及互斥锁 midpath 乐观自旋（都消耗 CPU，会出现在剖析里）。</details>

2. 分析 ext4 相关事件却不知道具体函数名时怎么办？
   <details><summary>答</summary>funccount 对 "ext4*" 通配计数，从调用频率切入定位相关函数。</details>
</details>
