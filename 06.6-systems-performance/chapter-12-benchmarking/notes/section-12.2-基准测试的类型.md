## 12.2 基准测试的类型

> 章节导航：[12.1 背景与挑战](./section-12.1-基准测试的背景与挑战.md) · 上一篇 ← · 下一篇 [12.3 基准测试方法论](./section-12.3-基准测试方法论.md) · [本章导读](../README.md)

**本节讲什么**：四种基准类型（Micro / Simulation / Replay / Industry Standard）的选择矩阵、各自的失真来源、replay 的「架构漂移」陷阱、HFT 各场景的类型选配。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 类型 = **保真度 ↔ 可控性** 的权衡 | 越像生产越不可控 |
| 2 | Micro 隔离变量，**测不到交互** | 锁竞争/NUMA/cache 争用 |
| 3 | Replay 最大的坑是**架构漂移** | 换盘/换栈后旧 trace 语义变了 |
| 4 | 行业标准适合**横向对比**，不适合证明你的生产性能 | 读它的方法论 |
| 5 | HFT 主力是 **Simulation + Replay 组合** | 合成压测找极限，真实回放做回归 |

---

### 一、保真度—可控性权衡图

```
可控性（可重复、单变量）
   ▲
   │ Micro          Simulation
   │  fio/iperf      合成 tick 流
   │
   │                Replay            Industry Std
   │                 历史 tick 回放     SPEC/TPC
   ▼
   └─────────────────────────────────────────▶ 保真度（像生产）
                       ▲
              HFT 回归检测的工作区间
```

四种类型没有高下，只有「回答什么问题」的区别：

| 类型 | 回答的问题 | 不回答的问题 |
|------|-----------|-------------|
| **Micro** | 这个组件的物理上限？ | 组件组合后表现？ |
| **Simulation** | 合成压力下系统拐点？ | 真实流量的长尾行为？ |
| **Replay** | 真实流量下端到端表现？ | 超出历史的新场景？ |
| **Industry** | 和别家机器比怎么样？ | 我们的生产会怎样？ |

### 二、微观基准（Micro-Benchmarking）

针对**单一组件**、简化人工负载：

| 组件 | 常见工具 | 测什么 |
|------|----------|--------|
| CPU | `perf bench`、自定义 loop | 算力、分支、syscall 开销 |
| 磁盘/FS | **fio** | IOPS、延迟分位（[ch9](../../chapter-09-disks/)） |
| 网络 | **iperf3**、netperf | 吞吐、RTT（[ch10](../../chapter-10-network/)） |
| 内存 | `lmbench`、`stream` | 带宽、load latency |
| DPDK | testpmd / pktgen | PPS、转发路径 |

- **优点**：隔离变量、可重复、快速迭代调优假设。
- **缺点**：**脱离**真实 syscall 路径、锁、业务逻辑——组件级快 ≠ 栈级快。

典型用法（fio 测日志盘延迟，参数即文档）：

```bash
fio --name=latency --filename=/dev/nvme0n1 --direct=1 \
    --rw=randwrite --bs=4k --iodepth=1 --runtime=60 \
    --group_reporting --percentile_list=50:99:99.9
# iodepth=1 测的是「单请求往返延迟」，iodepth=32 测的是「排队吞吐」——两个不同的问题
```

### 三、模拟测试（Simulation）

**合成 workload** 模仿生产特征（比例、大小、并发、到达率分布）——比 raw micro 接近真实，但**形状必须校准**。

校准清单（合成为什么可信）：

```
□ 到达率：固定速率还是突发（生产是泊松/突发？）
□ 大小分布：报文/请求大小的直方图（不是均值）
□ 读写比例：与生产统计对齐
□ 并发度：连接数/线程数/队列深度
□ 时间特征：日内高峰形态（开盘 15 分钟 vs 全天平均差 10×）
```

**HFT 例**：合成 UDP 组播行情流 + 固定 order book 深度做压力回归——能找到系统的拐点和退化形态，但**字段分布、包间 burstiness、symbol 相关性**仍不如真实 exchange feed。所以合成测拐点、真实测回归，两者都要。

### 四、重放测试（Replay）

捕获**生产 trace** 再回放：

| 类型 | 工具/方式 | HFT 场景 |
|------|-----------|---------|
| 块 I/O trace | `blktrace` 录制 + replay（[ch9](../../chapter-09-disks/)） | 日志盘验收 |
| 网络 pcap | tcpreplay | 历史行情回放 |
| 应用请求 | 自定义日志/二进制 replay | tick → 策略回归 |

**⭐ 架构漂移陷阱（Gregg 警告，ch9 呼应）**：replay 的前提是「**重放路径的行为特征与录制时相同**」。若目标系统架构或性能特征已变，replay 可能误导：

| 录制时的特征 | 换硬件/栈后 | 后果 |
|-------------|------------|------|
| HDD 队列合并模式 | 换 NVMe 后合并机会完全不同 | trace 里的 I/O 序列不再代表真实到达模式 |
| 内核栈 pacing | 换 DPDK 后无队列 | 网络回放的时间戳失去意义 |
| page cache 命中序列 | 换内存容量 | 命中/穿透模式重排 |

**规则：换后端（新盘/新 FS/新网卡/新栈）→ 重新在生产录 trace，别用旧的。**

历史 tick replay **测策略逻辑**不在此列——那是数据回放不是性能回放，测的是决策正确性；但**测系统延迟**时，replay 的注入节奏（一秒放多少 tick、burst 怎么分布）必须保真。

### 五、行业标准（Macro / Industry Standards）

| 套件 | 领域 | 用途 |
|------|------|------|
| SPEC CPU 2017 | CPU 编译/浮点 | 采购横向对比 |
| SPECjbb | Java 服务端 | JVM 平台 |
| TPC-C / TPC-E | 数据库 OLTP | DB 选型 |
| 厂商 NIC benchmark | 网络转发 | 上限参考 |

特点：宏观、可对比、**不一定**像你的业务。正确姿势：读它的**方法论**（怎么控制变量、怎么报分布），用 [12.4 拷问问题](./section-12.4-基准测试拷问Benchmark-Questions.md) 穿透厂商引用的分数。

### 六、HFT 场景 × 类型选配总表

| 场景 | 类型 | 工具/方式 | 判读重点 |
|------|------|----------|---------|
| 日志盘验收 | Micro | fio `direct=1` iodepth=1 | P999 + 掉毛频率 |
| 网络带宽 baseline | Micro | iperf3 单/多流分开报 | 多流是否线性扩展 |
| 网卡 PPS 上限 | Micro | testpmd / pktgen | 小包（64B）pps |
| 系统拐点 | Simulation | 合成 tick 流阶梯加压 | 拐点位置 + 退化形态 |
| 策略回归 | Replay | 历史 tick + 全分布对比 | P99/P999 漂移 |
| 端到端 SLA | Custom + Passive | 生产 span timestamp | 见 [14-HFT ch09](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md) |
| 整机采购 | Macro + Micro | SPEC + 自有 replay 两轮 | 两轮结论一致性 |

### 衔接

- 上一节：[12.1 背景与挑战](./section-12.1-基准测试的背景与挑战.md)（失败模式机制）
- 下一节：[12.3 基准测试方法论](./section-12.3-基准测试方法论.md)（阶梯负载 + sanity check）
- 拷问视角：[12.4 Benchmark Questions](./section-12.4-基准测试拷问Benchmark-Questions.md)

---

### 常见陷阱

1. **micro 结论当生产结论**——「fio 4k 随机写 100µs」不能推出「策略日志写 P99=100µs」，中间有 page cache、journal、邻居。
2. **simulation 不校准形状**——固定速率打流量测出的吞吐，遇到生产突发直接失效。
3. **换后端不重录 trace**——NVMe 上重放 HDD 时代的 blktrace，测的是「旧 trace 在新盘上的重排」，不是生产负载。
4. **行业标准分数直接套用**——SPEC 高分机器跑你的延迟敏感负载可能很差（SPEC 奖励吞吐，不奖励 tail）。

<details>
<summary>自测题（点击展开）</summary>

1. 四种基准类型各自回答什么问题？
   <details><summary>答</summary>Micro：组件物理上限；Simulation：合成压力下拐点；Replay：真实流量端到端表现；Industry：横向对比。没有一种能单独回答全部。</details>
2. replay 的架构漂移陷阱是什么？
   <details><summary>答</summary>replay 假设重放路径行为与录制时相同；换盘/FS/网卡/栈后，队列、合并、cache 特征全变，旧 trace 不再代表真实到达模式——换后端要重录。</details>
3. fio 的 iodepth=1 和 iodepth=32 测的是同一件事吗？
   <details><summary>答</summary>不是：iodepth=1 测单请求往返延迟（延迟敏感场景），iodepth=32 测排队下的吞吐上限（带宽场景）——报结果必须带 iodepth。</details>
4. 合成压测为什么要校准形状？
   <details><summary>答</summary>系统的退化形态由负载的时间特征（突发、分布）决定——固定速率测出的拐点在突发流量下不成立。</details>
5. 历史 tick replay 测策略和测系统延迟的区别？
   <details><summary>答</summary>测策略=数据回放（决策正确性，节奏无关）；测系统延迟=性能回放（注入节奏、burstiness 必须保真）。</details>

</details>


---

← [本章导读](../README.md)
