## 6.9 CPU 调优

> 章节导航：[6.8 实验工具](./section-6.8-实验工具.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：CPU 调优的优先级序（Gregg 顺序）、低延迟隔离栈的完整拼图（isolcpus/affinity/nohz_full/irq affinity）、每层手段的机制与陷阱。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 优先级：**消除工作 > 编译 > 调度 > 频率 > 绑核** | 越靠后越贵越脆弱 |
| 2 | HFT 裸机**隔离不用 quota** | quota 引入 throttling stall |
| 3 | 隔离是**一套拼图**不是单开关 | isolcpus + nohz_full + IRQ 迁移 + RT |
| 4 | governor=performance 是**入场券** | 省电模式的延迟不可预测 |
| 5 | RT 有**兜底旋钮** | sched_rt_runtime_us |

---

### 一、优先级（Gregg 顺序）

```
1. 消除不必要的工作     ← ROI 最高（Ch 5：算法、缓存、锁、批量）
2. 编译器优化            ← -O2/-O3、LTO、PGO（需 benchmark 验证）
3. 调度优先级            ← nice / chrt（RT 谨慎）
4. 频率                  ← governor = performance
5. CPU 绑定              ← taskset / isolcpus / cpusets
6. 资源控制              ← cgroups v2 quota（云/容器）
```

为什么绑核排第五：它不减少任何工作，只是**消除调度噪声**——如果工作本身多余（多余的 copy、多余的分支预测失败），绑核后的热核更快地做无用功。先减法后隔离。

### 二、调优手段对照

| 手段 | 命令/配置 | 机制 | HFT 场景 |
|------|-----------|------|----------|
| **nice** | `nice -n 19` | CFS 权重（vruntime 增速） | 监控进程调低 |
| **chrt -f** | `chrt -f 80` | SCHED_FIFO RT 优先级（抢占 CFS） | 仅关键线程 + 文档化 |
| **cpufreq** | `cpupower frequency-set -g performance` | 定频不降频 | 裸机默认 |
| **taskset** | `taskset -c 2 ./strategy` | 设置亲和掩码 | 进程启动时绑定 |
| **isolcpus** | 内核参数 `isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3` | 核从调度器可用集移除 | 数据面专用核 |
| **cpusets** | cgroup cpuset | 组级亲和 | 容器化部署 |
| **cgroups CPU** | `cpu.max` | 周期配额 | 多租户；低延迟**慎用** |
| **irqbalance 关** | systemctl stop irqbalance + 手动 `/proc/irq/*/smp_affinity` | 中断固定到 housekeeping 核 | 网卡 interrupt affinity |
| **RPS/XPS** | `/sys/class/net/eth*/queues/*/rps_cpus` | 软中断分散 | 与 DPDK 轮询互斥 |

### 三、⭐ 低延迟隔离栈（完整拼图）

单个手段都不够——**隔离是一套组合**，每块挡一种噪声源：

```
噪声源                          → 拦截手段
─────────────────────────────────────────────────────
其他进程被调度到热核             → isolcpus=2,3（核退出通用调度）
内核周期性 tick（HZ 中断）       → nohz_full=2,3（进入自适应无 tick）
RCU 回调                        → rcu_nocbs=2,3（回调搬到别的核）
网卡/定时器/其它硬件中断         → IRQ affinity 手动固定到 housekeeping 核
工作线程唤醒延迟（CFS 排队）     → SCHED_FIFO/chrt（关键线程 RT 化）
kswapd/flush 等内核线程         → cgroup cpuset 排除热核（kthread 亲和）
频率波动（P-state 切换）        → governor=performance（定频）
C-states（睡眠唤醒延迟）        → max_cstate=1 / idle=poll（按延迟预算）
cache/带宽串扰（邻居核）        → 物理隔离（热核间空核缓冲，[6.8 实测法](./section-6.8-实验工具.md)）
```

**验证**（每块拼图都要有证据，不能装完就信）：

```bash
# isolcpus 生效：热核上无其他进程
ps -eLo psr,pid,comm | awk '$1==2 || $1==3'    # 只应看到策略线程
# nohz 生效：热核的中断计数几乎不涨
watch -d cat /proc/interrupts
# 调度延迟：runqlat 热核直方图应全部 < 数十 µs
sudo runqlat-bpfcc 10
# 定频生效
cat /proc/cpuinfo | grep MHz | sort | uniq -c   # 所有核恒定最高频
```

### 四、关键陷阱的机制解释

**① quota 的 throttling stall**（HFT 裸机不用 quota 的原因）：

`cpu.max` 按周期发配额，用完**整个 cgroup 冻结到下周期**（[ch11.3 机制图](../../chapter-11-cloud-computing/notes/section-11.3-操作系统虚拟化-容器.md)）——即使 CPU 全闲着也冻结。isolcpus 是物理隔离，无配额无冻结；**隔离（isolcpus）与限流（quota）是两种正交工具**，HFT 要前者。

**② SCHED_FIFO 无上限 = 系统锁死风险**：

RT 线程死循环不主动让出 → CFS 全饿死 → 看门狗都跑不动。兜底：

```bash
cat /proc/sys/kernel/sched_rt_runtime_us    # 默认 950000（每 1s 周期 RT 最多占 950ms）
cat /proc/sys/kernel/sched_rt_period_us     # 1000000
```

RT throttling 保证系统不被 RT 饿死——**别改成 -1 关闭**；HFT 的正确姿势是关键线程 FIFO + 确保它们会阻塞等待事件（epoll/tick），不是纯自旋。

**③ governor != performance 的隐性延迟**：

schedutil/ondemand 按负载调频——低负载时降频，tick 到来要处理行情时**频率还没爬上来**（µs~ms 级爬坡）；省电模式还有 C-state 唤醒延迟。低延迟要求**恒频 + 浅睡**：`performance` governor + `max_cstate=1`（或 idle=poll 极端档，烧 CPU 换 µs）。

### 五、与相邻章的协作

| 问题 | 在哪解决 |
|------|---------|
| 热点代码本身太慢 | [Ch5 应用](../../chapter-05-applications/)（算法/锁/伪共享）+ [ch13 perf](../../chapter-13-perf/) 定位 |
| cache miss 高 | [Ch7 内存](../../chapter-07-memory/) + [06-linux-mm](../../../06-linux-mm/)（数据结构布局） |
| 调度延迟来自哪 | [ch15 runqlat](../../chapter-15-bpf/) + 本节隔离栈 |
| 验证调优真有效 | [Ch12 对照实验](../../chapter-12-benchmarking/) + 优化前后 perf stat/runqlat |

**验证闭环**：应用层改（ch5）→ 这里绑核隔离 → 用 mpstat + perf + runqlat 验证「CPU 与 run queue 真降/稳了」——没有验证的调优是迷信。

### HFT / 嵌入式关联

- **隔离栈是 HFT 裸机的开机配置**：isolcpus/nohz_full/rcu_nocbs/IRQ 迁移/governor 打包成 kernel cmdline + systemd 单元，装完跑验证清单（本节三）。
- **实时性来源的层级**：吞吐优化（算法/编译）改善均值；**隔离栈改善尾部**——P99/P999 的稳定性主要靠后者。
- **嵌入式 RT**：PREEMPT_RT 补丁 + 本节同一套隔离思路（车载/工控的实时核分配）；`isolcpus` 与 RT 补丁的优先级模型是互补关系。
- **云上退化形态**：拿不到 isolcpus（改不了宿主 cmdline）时退化用 cpuset + taskset + irq affinity（仅 guest 内）——效果打折，这是云与裸机的延迟差距来源之一（[ch11](../../chapter-11-cloud-computing/)）。

### 衔接

- 上一节：[6.8 实验工具](./section-6.8-实验工具.md)（隔离验证的实验方法）
- 关联：[Ch5 应用层优化](../../chapter-05-applications/)、[Ch7 内存](../../chapter-07-memory/)、[ch11 cgroup 机制](../../chapter-11-cloud-computing/notes/section-11.3-操作系统虚拟化-容器.md)、[ch12 对照实验](../../chapter-12-benchmarking/)、[14-HFT ch05/ch06](../../../14-hft-engineering/)、[06-linux-mm 调度相关](../../../06-linux-mm/)

---

### 常见陷阱

1. **调优从绑核开始**——先消除不必要工作（ROI 最高），最后才绑核。
2. **RT 优先级不设上限**——SCHED_FIFO 无 cap 饿死 CFS；保留 sched_rt_runtime_us 兜底。
3. **cgroup quota 用在裸机热路径**——quota 冻结引入 stall；HFT 用隔离不用限流。
4. **只 isolcpus 不迁 IRQ**——中断还是打到热核（打断了 nohz）；隔离是拼图不是开关。
5. **不验证就宣布隔离完成**——ps/interrupts/runqlat 三件验证一个都不能少。

<details>
<summary>自测题（点击展开）</summary>

1. CPU 调优的优先级顺序？
   <details><summary>答</summary>1) 消除不必要工作 2) 编译优化 3) 优先级 4) 频率 governor 5) 绑核 6) cgroup 资源控制——越靠后越贵且不减少工作量。</details>
2. 隔离栈的完整拼图有哪些块？
   <details><summary>答</summary>isolcpus（调度隔离）+ nohz_full（tick）+ rcu_nocbs（RCU 回调）+ IRQ affinity（中断迁走）+ SCHED_FIFO（唤醒优先）+ performance governor（定频）+ cstate 限制 + 物理空核缓冲（cache 串扰）——每块拦一种噪声。</details>
3. 为什么 HFT 裸机用 isolcpus 而非 quota？
   <details><summary>答</summary>quota 是周期配额+冻结机制（用完强制休眠到下周期），即使 CPU 空闲也冻结；isolcpus 物理隔离无配额概念——隔离与限流正交，低延迟要前者。</details>
4. SCHED_FIFO 的兜底机制？
   <details><summary>答</summary>sched_rt_runtime_us/period_us（默认 950ms/1s）——RT 总占比受限，保证 CFS/内核线程不被饿死；正确姿势是关键线程阻塞等事件而非自旋。</details>
5. governor=performance 为什么是低延迟入场券？
   <details><summary>答</summary>动态调频在低负载时降频，事件到来时频率未爬满（µs~ms 爬坡）；C-state 唤醒也有延迟——恒频+浅睡才可预测。</details>

</details>


---

← [本章导读](../README.md)
