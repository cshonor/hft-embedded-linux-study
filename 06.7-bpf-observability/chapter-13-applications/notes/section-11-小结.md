# 11. 小结（13.5）

> 底本：《BPF之巅》第 13 章 应用程序，13.5 节（印刷 p664）

## 原书小结

本章介绍了前面面向资源的章节（第 6–10 章）没讲到的 BPF 工具，用于**应用程序分析**，覆盖：

- **应用程序上下文**（以 MySQL 为示例应用，通过 USDT 探针和 uprobes 读取查询语句上下文）
- **线程使用**
- **信号**
- **锁**
- **睡眠**

并由于 MySQL 服务器的重要性，再次使用 BPF 工具进行了 **on-CPU 和 off-CPU 分析**。

## 本章工具全景（按主题）

| 主题 | 工具 | 开销 |
|------|------|------|
| CPU 分析 | profile、threaded、syscount | 低（采样） |
| off-CPU 分析 | offcputime、offcpuhist、ioprofile | **高（>5%，短时运行）** |
| 应用上下文 | mysqld_qslower、mysqld_clat | 可忽略（请求/命令频率低） |
| 线程执行 | execsnoop、threadsnoop | 可忽略 |
| 锁分析 | pmlock、pmheld | **高（锁频率 ~10 万/秒）** |
| 信号 | signals、killsnoop | 可忽略 |
| 睡眠分析 | naptime | 可忽略 |
| 死锁 | deadlock(8)（BCC，锁序倒置有向图） | 可能很高 |
| 栈前提 | libc 帧指针（无则栈断在 libc） | — |

## 方法论回顾

1. 先判**线程模型**（服务线程池/CPU 线程池/事件处理器/SEDA）→ 决定 tid 能否做请求关联键
2. **十步策略**：工作单元 → 组件信息 → 后台任务 → USDT 检查 → on-CPU → off-CPU → syscount → 资源章工具 → uprobes → 分布式双端
3. USDT 优于 uprobes（版本稳定性）；USDT 不可用时 uprobes 版工具是备胎（diff 改写往往只有几行）
4. offcputime 输出被等待线程占据是正常的——**盯住处理请求路径上的阻塞栈**

## HFT 关联

本章对交易系统的核心启示：

- **归因链闭环**：资源指标（I/O 延迟、CPU）+ 业务上下文（订单/查询/策略名）在同一张表里 = 可行动的性能报告；tid 关联法是粘合剂
- **开销分级决定使用时机**：请求级（qslower 类）与采样类（profile/threaded）可常驻交易时段；off-CPU 与锁类只在演练环境或短窗口用
- **三大坑**：libc 无帧指针栈断裂、事件处理器模型 tid 不可做键、发行版 MySQL 无 USDT 探针——每个都有第 13 章给出的替代方案

<details>
<summary>自测题</summary>

1. 本章覆盖了哪五个面向资源的章节未涉及的应用行为？
   <details><summary>答</summary>应用程序上下文、线程使用、信号、锁、睡眠。</details>

2. 哪两类工具开销大到必须限时运行？
   <details><summary>答</summary>off-CPU 类（offcputime/offcpuhist/ioprofile，随上下文切换率可超 5%）和锁类（pmlock/pmheld，锁事件每秒近 10 万次）。</details>
</details>
