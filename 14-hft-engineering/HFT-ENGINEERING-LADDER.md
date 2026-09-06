# HFT 工程能力阶梯：从 C/C++、Linux 底层到低延迟系统开发

**文件夹 16（14-hft-engineering）** · [返回模块索引](./README.md) · [返回总清单](../READING-LIST.md)

> **本文定位：** 仓库里已经有 [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)（**读什么书、按什么顺序**），
> 本文补的是另一半——**做什么项目、按什么指标验收**。  
> 一句话区分：**阅读路线管「懂不懂」，工程阶梯管「做没做出来」**。
>
> **目标岗位：** HFT **低延迟基础设施**（行情接收、订单网关、撮合引擎、网络/内存热路径），
> **不是**量化研究员——所以数学建模那一类（随机过程、最优执行）**后置不前置**。

---

## 一、为什么必须单列一条「工程线」

| 只读不动手 | 后果 |
|------------|------|
| 读完《Systems Performance》但没 profile 过自己的程序 | 工具名背熟了，看到火焰图仍然不知道该改哪一行 |
| 读完 LKD 但没绑过核 | 「`isolcpus` + `SCHED_FIFO`」只是名词，不知道 p99 掉了多少 |
| 读完 Hennessy Ch2 但没写过无锁队列 | 伪共享只是概念，没见过 padding 前后差 3 倍的实测 |
| 读完 Harris 但没写过 LOB | 说得出 price-time priority，写不出撮合循环 |

> ⚠️ **工业级 HFT 系统没有任何一本书能教会你。** 书只能给你「该往哪看」，
> 真正的手感来自：**写 → 量 → 改 → 再量** 的循环。本文每一级都强制配一个可量测的交付物。

---

## 二、阶梯总览

```
L0  C 语言与指针        ── 能手写 malloc / 解释 struct 对齐
     ↓
L1  用户态系统编程      ── epoll + mmap + 线程 + RT 调度（TLPI）
     ↓
L2  内核机制与内存      ── CFS / 中断 / VMA / 页表 / slab（LKD + Gorman）
     ↓
L3  网络与协议栈        ── UDP 组播 / TCP 选项 / NAPI / 内核收包路径
     ↓
L4  低延迟专项          ── cache / NUMA / 内存序 / 无锁 / 绑核 / 大页
     ↓
L5  系统整合            ── 内核旁路 + 撮合引擎 + PTP 时钟 + 全链路测量
```

| 级 | 对应模块 | 主线知识 | 交付项目 | 硬验收指标 |
|----|---------|---------|---------|-----------|
| **L0** | [`01` C 语言](../01-c-language/) · [`02` CSAPP](../02-computer-systems/) | 指针算术、struct 布局、堆分配、ABI | 自实现 `malloc` + 对齐/合并 benchmark | 能解释 chunk header / bins / `M_MMAP_THRESHOLD` |
| **L1** | [`03` TLPI](../03-linux-userspace-api/) · [`03.5` UNP](../03.5-unix-network-api/) · [`03.6` 调试](../03.6-userspace-debugging/) · [`04` C++](../04-cpp/) | TLPI：fd、线程、mmap、信号、epoll | 多线程 TCP echo server（epoll ET + 线程池） | p99 < 200μs；能画出请求完整路径 |
| **L2** | [`05` LKD](../05-linux-kernel/) · [`05.5`](../05.5-modern-kernel/) [`05.6`](../05.6-kernel-debugging/) · [`06` MM](../06-linux-mm/) · [`06.5`](../06.5-modern-mm/) · [`06.6` SysPerf](../06.6-systems-performance/) | LKD + Gorman：调度/中断/VMA/页表/slab | `perf` 定位并消除一次真实抖动 | 能用火焰图 + `perf stat` 说清瓶颈归属 |
| **L3** | [`11` TCP/IP](../11-tcpip-protocols/) · [`11.5` 抓包](../11.5-wireshark-packet-analysis/) · [`12` 内核网](../12-kernel-networking/) · [`12.5` 现代网络](../12.5-modern-networking/) | 组播、UDP、socket 选项、NAPI | UDP 组播行情接收器（含丢包统计） | 10 万 pps 下 **零丢包**，能说出丢包在哪一层 |
| **L4** | [`15` 体系结构](../15-computer-architecture/) · [`07` ARM64](../07-arm-architecture/) · [`02` CSAPP](../02-computer-systems/) · [ch07 无锁](./chapter-07-lockless-data-structures-memory-layout/README.md) | cache line / NUMA / 内存序 / 无锁 | SPSC 无锁 ring（padding 前后对比） | 单跳 < 100ns，p99 < 200ns，**批量消费**版更快 |
| **L5** | [`13` DPDK](../13-dpdk/) · [本模块 ch06/ch09/ch13](./README.md) · [`19` 微观结构](../19-markets-microstructure/) · [`06.7` BPF](../06.7-bpf-observability/) | DPDK/AF_XDP、LOB、PTP、T2T 测量 | 三进程：FeedHandler → Book → Strategy | 软件栈 tick-to-trade **p99 < 10μs**（自测环境如实记录） |

---

## 三、L0 · C 语言与指针

> **对应模块：** [`01` C 语言](../01-c-language/)（K&R / Pointers on C / Expert C / Modern C / 嵌入式 C 自我修养 / C 陷阱）· [`02` CSAPP](../02-computer-systems/) Ch3 Ch6 Ch9

### 知识点

| 主题 | 要追到的深度 | 为什么 HFT 需要 |
|------|-------------|----------------|
| 指针算术与数组退化 | `*(a+i)` 的寻址展开、越界不检查 | 二进制协议解析全靠指针挪动 |
| struct 布局与对齐 | 对齐规则、尾部填充、`offsetof` | 一个 64B cache line 能放几个订单 |
| 函数指针与回调 | 跳转表、间接调用开销（分支预测失败 ~15-20 cycle） | 消息分发表的选型 |
| 堆分配 | `malloc` 的 chunk header、bins、`brk` vs `mmap` 分界 | 热路径**禁用** malloc 的理由 |
| stream 与 FILE | `FILE` 对象、`_IO_write_ptr`、缓冲模式 | 日志不能阻塞热路径的根本原因 |
| ABI / 调用约定 | 寄存器传参、栈帧、返回值位置 | 读 perf 火焰图里的汇编 |

### 交付项目

**P0-1：自实现 `malloc`**（隐式空闲链表 → 分离空闲链表两版）
- 对齐到 16B；块头记录 size + 使用位
- 释放时合并相邻空闲块
- benchmark：随机 alloc/free 10 万次，对比 glibc `malloc`

**P0-2：`struct` 布局实验**
- 打印 `sizeof` / `offsetof`，画出内存布局 ASCII 图
- 同一组字段按两种顺序排列，对比 `sizeof`（预期差 30%+）

### 验收

- [ ] 能解释：为什么 `malloc(0)` 返回非空、为什么 `free` 后指针还能读
- [ ] 能解释：`brk` vs `mmap` 的 128KB 分界线，以及超过后为什么整块归还
- [ ] 能解释：为什么 `realloc` 可能原地扩容也可能搬迁（热路径不能依赖）

> 📌 **《Pointers on C》§7.1（stream model / FILE 对象）落在这一级**——
> 看似只是 I/O，实际是「用户态缓冲 vs 系统调用」的第一课，**直接决定后面日志怎么写**。

---

## 四、L1 · 用户态系统编程（TLPI 主线）

> **对应模块：** [`03` TLPI](../03-linux-userspace-api/)（主线）· [`03.5` UNP](../03.5-unix-network-api/) · [`03.6` 用户态调试](../03.6-userspace-debugging/) · [`04` C++](../04-cpp/)

### 高价值章节（HFT 视角重排，非原书顺序）

| 优先级 | TLPI 章节 | 核心 API | HFT 用途 |
|--------|----------|---------|---------|
| 🔴 P0 | Ch 63 备选 I/O 模型 | `epoll`（LT/ET） | 多路行情接入骨架 |
| 🔴 P0 | Ch 49 内存映射 | `mmap`、`MAP_HUGETLB` | 共享内存 IPC、订单簿预分配 |
| 🔴 P0 | Ch 29–33 线程 | `pthread`、同步原语 | 线程布局与绑核前提 |
| 🔴 P0 | Ch 35 进程优先级与调度 | `SCHED_FIFO`、`sched_setaffinity` | 策略线程隔离 |
| 🟠 P1 | Ch 20–22 信号 | `sigaction`、`signalfd` | 热路径屏蔽、优雅退出 |
| 🟠 P1 | Ch 58–61 Socket | 非阻塞、`TCP_NODELAY` | 订单链路 |
| 🟠 P1 | Ch 50 虚拟内存操作 | `mlock`、`madvise` | 防 swap、预触碰页表 |
| 🟡 P2 | Ch 44–48 / 51–55 IPC | 管道、POSIX shm | 模块间通信 |
| ⚪ | 文件属性 / 凭证 / 终端 | — | 非热路径 |

### 交付项目

**P1-1：epoll + 线程池 TCP echo server**
- epoll ET 模式，非阻塞 socket，单 acceptor + N worker
- 每个连接独立读缓冲，处理粘包
- 记录每条请求的 p50/p99/p999

**P1-2：共享内存 + 无锁传递**（为 L4 铺垫）
- `mmap(MAP_SHARED|MAP_ANONYMOUS)` 建共享区
- 两个进程通过共享区传消息，对比管道/socket 的延迟（**量级差 10–50 倍**）

### 验收

- [ ] 能解释 ET 与 LT 的**分叉点在 `ep_send_events`**，不在回调（v6.6 实证）
- [ ] 能解释为什么 `epoll_ctl` 是 O(log n)（`rb_root_cached`）而不是 O(1)
- [ ] 实测：`mlock` 前后、页表预触碰前后的首次访问延迟差

---

## 五、L2 · 内核机制与内存

> **对应模块：** [`05` LKD](../05-linux-kernel/) · [`05.5` 现代内核](../05.5-modern-kernel/) · [`05.6` 内核调试](../05.6-kernel-debugging/) · [`06` Gorman MM](../06-linux-mm/) · [`06.5` 现代 MM](../06.5-modern-mm/) · [`06.6` SysPerf](../06.6-systems-performance/)（perf 方法论）

### 必须吃透的六个机制

| 机制 | 关键问题 | 对延迟的影响 |
|------|---------|-------------|
| **CFS / RT 调度** | 我的线程多久能被调度上？ | 抢不到核 = 直接抖动 |
| **中断 / softirq** | 网卡中断打在哪个核上？ | 打断策略线程 = p99 尖刺 |
| **上下文切换** | 切换一次丢多少？TLB 要不要刷？ | 每次 ~μs 级，且**方差巨大** |
| **VMA / 页表** | 首次访问为什么慢？ | 缺页 = 几 μs 起跳 |
| **slab / 内存池** | 热路径能 malloc 吗？ | 不能——用预分配池 |
| **NUMA** | 我的内存和我在同一个 node 吗？ | 跨 socket 访存接近翻倍 |

### 交付项目

**P2-1：抖动归因实验**
- 写一个死循环忙等的线程，用 `perf sched` / `perf record` 抓它被谁打断
- 关掉 `irqbalance`、把网卡中断挪到别的核，对比 p99 变化
- **记录真实数字**（你的机器上的，不是书上的）

**P2-2：缺页延迟对照**
- `mmap` 512MB，**不触碰** vs **逐页写一遍** 后访问，对比首次访问延迟
- 这是「COW 省物理页不省页表结构」的最佳实证

### 验收

- [ ] 能画出：网卡 DMA → 硬中断 → softirq/NAPI → 协议栈 → socket → 用户态
- [ ] 能解释：`isolcpus` / `nohz_full` / `rcu_nocbs` 各自关掉了什么
- [ ] 能用 `numactl --membind` 证明跨 node 访存更慢

> 📌 **LKD 批 F（Ch14 块 IO + Ch16 页缓存）在这一级，但对 HFT 是 🟡 非热路径**——
> 交易系统热路径不走磁盘。价值在于**对照**：理解内核为什么要把磁盘路径做得那么复杂，
> 才明白旁路网络栈省掉的是哪一段。

---

## 六、L3 · 网络与协议栈

> **对应模块：** [`11` TCP/IP](../11-tcpip-protocols/) · [`11.5` 抓包](../11.5-wireshark-packet-analysis/) · [`12` 内核网络](../12-kernel-networking/) · [`12.5` 现代网络](../12.5-modern-networking/)（XDP / NAPI / AF_XDP）

### 知识地图

| 层 | 要掌握 | 关键旋钮 |
|----|--------|---------|
| **物理/链路** | 共置、交换机 cut-through、光纤 vs 微波 | 跳数、线长 |
| **IP/UDP** | 组播、IGMP、避免分片 | `DF` 位、MTU |
| **TCP** | 握手、拥塞、Nagle | `TCP_NODELAY`、`TCP_QUICKACK` |
| **Socket** | 缓冲区、忙轮询 | `SO_RCVBUF`、`SO_BUSY_POLL`、`SO_REUSEPORT` |
| **内核路径** | NAPI、RSS/RPS/XPS | 队列绑核 |
| **旁路** | DPDK / Onload / AF_XDP / RDMA | 见 L5 |

### 交付项目

**P3-1：UDP 组播行情接收器**
- 加入组播组，非阻塞 `recvmsg` 批量收
- 统计：接收 pps、丢包数、gap（序列号跳变）
- 排查工具链：`ethtool -S`（网卡丢）、`/proc/net/softnet_stat`（软中断丢）、`ss -uln`（socket 队列丢）

**P3-2：分层丢包定位**
- 人为打满（提高发送速率），观察丢包**最先出现在哪一层**
- 逐层调：`SO_RCVBUF` → RSS 队列数 → 中断亲和 → busy poll

### 验收

- [ ] 10 万 pps 下零丢包，且能说清「我调了哪三个参数」
- [ ] 能解释为什么**行情用 UDP 组播、订单用 TCP**（可靠性 vs 延迟的取舍）
- [ ] 能解释 `SO_BUSY_POLL` 省了什么、代价是什么（CPU 100%）

---

## 七、L4 · 低延迟专项（HFT 的核心竞争力）

> **对应模块：** [`15` 体系结构](../15-computer-architecture/)（cache / MESI）· [`07` ARM64](../07-arm-architecture/)（弱内存序）· [`02` CSAPP](../02-computer-systems/) Ch6 Ch12 · [本模块 ch07 无锁与内存布局](./chapter-07-lockless-data-structures-memory-layout/README.md)

这是**唯一别人替代不了你**的一级。前面三级是通用系统能力，这一级是 HFT 专属。

### 7.1 cache 与内存布局

| 概念 | 数字 | 工程动作 |
|------|------|---------|
| L1d 命中 | ~1ns（~4 cycle） | 热数据 ≤ 32KB |
| L2 命中 | ~4ns | — |
| L3 命中 | ~12–15ns | 跨核共享代价 |
| 主存（本地 NUMA） | ~80ns | — |
| 主存（跨 socket） | ~130–150ns | `numactl --membind` |
| **cache line** | **64B** | 结构体对齐 + padding |
| **伪共享** | 两个核写同一 line | 每次 ~几十 ns 的 line ping-pong |

**必做实验：** 两个线程各写一个 `volatile` 计数器
- A：相邻（同一 cache line）→ 慢 3–5 倍
- B：`alignas(64)` 分开 → 正常
- **这就是 `alignas(64)` 存在的全部理由**

### 7.2 内存序（ARM64 汇编在这里直接变现）

| 架构 | 内存模型 | 意味着 |
|------|---------|--------|
| **x86-64** | **TSO**（强序） | 只有 store→load 会重排；`acquire/release` 编译成**零指令** |
| **ARM64** | **弱序** | 读写任意重排；`acquire/release` 要 `ldar`/`stlr` |
| POWER | 更弱 | — |

> ⭐ **`07-arm-architecture` 的 ARM64 汇编，在这里第一次产生 HFT 价值**：
> 在 x86 上「忘记写 memory_order」往往**碰巧能跑**；在 ARM64 上会**直接崩**。
> 能解释为什么 = 真正理解了内存序，而不是背了 C++ 的六个枚举。

**要点：**
- `memory_order_relaxed`：只保证原子性，不保证顺序 → 计数器
- `acquire/release`：单向屏障 → 生产者/消费者（SPSC 的正确性来源）
- `seq_cst`：全序 → **默认但最贵**，x86 上是一条 `mfence`/`xchg`（~20-30 cycle）

### 7.3 无锁数据结构

| 结构 | 适用 | 关键技巧 |
|------|------|---------|
| **SPSC ring**（LMAX Disruptor 模式） | 单生产单消费，最常用 | 单调序号 + `& (size-1)` 取模（size 取 2 的幂）+ cache line padding + **批量消费** |
| MPMC ring | 多对多 | 序号 CAS，复杂度陡增；能不写就不写 |
| 无锁栈 | 少用 | ABA 问题 → tagged pointer |

**必做实验：** SPSC ring
- 正确版（`acquire/release`）vs 裸版（无屏障）→ 在 x86 上裸版可能**跑一万次都对**，换 ARM64 或加压力就崩
- padding 前后延迟对比
- 单条消费 vs 批量消费（**批量能摊掉屏障开销**）

### 7.4 系统调用与时钟

| 操作 | 典型开销 | 规避手段 |
|------|---------|---------|
| 空 syscall | 数十 ns ~ 数百 ns（**受 Spectre/Meltdown 缓解影响极大**） | 热路径不进内核 |
| `clock_gettime` via **vDSO** | ~20–25ns | `CLOCK_MONOTONIC` + vDSO |
| 上下文切换 | ~1–3μs（方差大） | 绑核、RT 调度、无阻塞 |
| 缺页 | ~μs 级 | 预分配 + `mlock` + 预触碰 |

> ⚠️ **上表所有数字都是「典型量级」，随 CPU 型号、内核版本、缓解开关变化极大。**
> **你必须自己测**——本仓库的纪律是「延迟数据要给可复现的量测方法，而不是只给结论数字」。

### 7.5 OS 与 BIOS 调优清单

| 项 | 动作 | 目的 |
|----|------|------|
| CPU | 关 Turbo / 关 C-State / 固定频 | 消除频率抖动（**延迟方差比均值重要**） |
| 隔离 | `isolcpus` + `nohz_full` + `rcu_nocbs` | 策略核上没有内核噪声 |
| 中断 | 关 `irqbalance`、手动 `smp_affinity` | 中断不打策略核 |
| 内存 | hugepage、`mlock`、预触碰 | 消除缺页和 TLB miss |
| NUMA | `numactl --cpunodebind --membind` | 本地访存 |
| 调度 | `SCHED_FIFO` + 优先级 | 不被抢占 |

### 交付项目

**P4-1：SPSC 无锁 ring + benchmark**
- 单跳 p50 < 100ns、p99 < 200ns（自测环境如实记录）
- 输出：padding 前/后、批量/单条、relaxed/acq_rel 三组对比

**P4-2：绑核前后对比**
- 同一程序：`taskset` 前后 + `SCHED_FIFO` 前后 + `isolcpus` 前后
- **输出 p50/p99/p999/max 四列**，看方差怎么变

---

## 八、L5 · 系统整合

> **对应模块：** [`13` DPDK](../13-dpdk/) · 本模块 [ch06 网络](./chapter-06-low-latency-network-protocol/README.md) / [ch09 测量](./chapter-09-latency-measurement-benchmarking/README.md) / [ch13 FPGA](./chapter-13-fpga-crypto-hft/README.md) · [`19` 市场微观结构](../19-markets-microstructure/) · [`06.7` BPF](../06.7-bpf-observability/)

### 8.1 内核旁路选型

| 方案 | 延迟 | 代价 | 适用 |
|------|------|------|------|
| **内核栈 + 调优** | 数十 μs | 零改造 | 起步、非极致场景 |
| **AF_XDP** | 个位数 μs | 需驱动支持，仍在内核 | 平衡方案（**你的 eBPF/XDP 学习在这里变现**） |
| **DPDK** | ~μs 级 | 独占网卡、要自己实现协议 | 极致软件方案 |
| **Solarflare Onload / EFVI** | 亚 μs ~ μs | 绑特定网卡 | 行业主流 |
| **RDMA / RoCE** | 亚 μs | 需对端配合 | 内部互联 |
| **FPGA** | 百 ns 级 | 开发成本极高 | 头部玩家 |

### 8.2 撮合引擎 / LOB

| 设计点 | 选择 |
|--------|------|
| 数据结构 | 价格档位数组（连续价格）或 map + 每档**侵入式链表** |
| 订单对象 | **预分配内存池 + 索引**，绝不在热路径 new |
| 撮合规则 | 价格优先 → 时间优先（FIFO） |
| 序号 | 单调递增，避免时间戳比较 |

### 8.3 时钟与时间测量

| 层 | 精度 | 说明 |
|----|------|------|
| **网卡硬件时间戳** | ~ns | 最准，PTP 同步 |
| 内核软件时间戳 | ~μs | 有协议栈排队误差 |
| 用户态 `clock_gettime` | ~20ns + 调度误差 | 最常用，需 vDSO |

**T2T（tick-to-trade）分段测量：** 进网卡 → 解包 → 更新 LOB → 决策 → 发单 → 出网卡，**每段打点**。
只报平均延迟是自欺——**必须看 p99 / p999 / max 的分布（CDF 图）**。

### 交付项目

**P5-1：三进程交易链路**
```
FeedHandler（组播收包+解码）→ [无锁 ring] → Book（维护 LOB）→ [无锁 ring] → Strategy（决策+发单）
      绑核 2                              绑核 3                          绑核 4
                    全链路 PTP 硬件时间戳 + 分段打点
```
- 验收：软件栈 tick-to-trade **p99 < 10μs**（如实记录环境：CPU 型号、网卡、是否旁路）
- 输出：CDF 图 + 分段延迟占比表

---

## 九、六本原版书该嵌在哪一级

| 书 | 类别 | 嵌入级别 | 怎么读 |
|----|------|---------|-------|
| **Low-Latency C++ Programming**（Antony Williams） | 低延迟系统工程 | **L4 主教材** | 与 P4-1/P4-2 同步做，读到哪做到哪 |
| **Building Low Latency Applications with C++**（Sourav Ghosh） | 低延迟系统工程 | **L5 项目参考** | 做 P5-1 时按章节对照，别通读 |
| **Developing High-Frequency Trading Systems**（Donadio） | 全景架构 | **L3–L5 穿插** | 先通读建立全景，做项目时回查 |
| **Trading and Exchanges**（Larry Harris） | 市场微观结构 | **L0 就可以开始** | 业务地基，越早越好（已在 `19-markets-microstructure`） |
| **High-Frequency Trading**（Irene Aldridge） | 策略与业务 | **L5 之后补** | 理解上层业务，不阻塞底层开发 |
| **Algorithmic and High-Frequency Trading**（Cartea 等） | 策略数学建模 | **暂缓** | 目标底层开发，随机微分方程不前置 |
| **Flash Boys**（Michael Lewis） | 行业纪实 | 任意空隙 | 无代码，理解行业生态 |

### 阅读优先级

| 梯队 | 书 | 触发条件 |
|------|----|---------|
| **第一梯队** | `Trading and Exchanges`（业务地基）+ `Low-Latency C++ Programming`（底层 C++） | 无前置，可先读 |
| **第二梯队** | `Building Low Latency Applications with C++` | 开始做 P5-1 三进程链路时 |
| **第三梯队** | `Developing HFT Systems`（架构全景）、`HFT (Aldridge)`（业务） | L3 网络之后 |
| **后置** | `Algorithmic and HFT`（数学） | 除非转向策略研究 |
| **消遣** | `Flash Boys` | 随时 |

---

## 十、贯穿全程的三条铁律

### 1. 不能测就没有优化

| 陷阱 | 症状 | 解法 |
|------|------|------|
| 编译器把 benchmark 优化掉 | 结果快得离谱 | 结果必须被消费（`volatile` 或输出） |
| Turbo / 降频 | 同一程序两次跑差 30% | 固定频率 + 多次取最小值分布 |
| 只报平均值 | 优化后 p50 好了 p999 炸了 | **永远看 p99/p999/max + CDF** |
| 首次缺页 | 第一次跑慢 10 倍 | 预热 + `mlock` + 预触碰 |
| ASLR / 布局 | 结果不稳定 | 多次运行取分布 |

### 2. 工具链要提前熟

`perf` / `perf stat` / 火焰图 · `ftrace` · `bpftrace`（抖动归因）·
`numactl` / `hwloc` · `taskset` · `ethtool` · `google-benchmark` ·
Intel PCM / VTune · `likwid`（cache 与 NUMA 计数）

> ⭐ **`06.7-bpf-observability` 那一整个模块，在 L4/L5 才第一次有真实靶子**——
> 这就是为什么仓库把它排在 `05`/`06` 之后。

### 3. 热路径四条禁令

```
① 不 malloc / free        → 预分配内存池
② 不 syscall              → 预分配 + 批量 + vDSO
③ 不加锁                  → 无锁 ring / 单线程 + 消息传递
④ 不 I/O（含日志）         → 异步日志 ring + 独立写盘线程
```

---

## 十一、简历级项目清单（做完这五个就够敲门）

| # | 项目 | 展示的能力 | 对应级别 |
|---|------|-----------|---------|
| **P1** | 自实现 malloc + 内存池 | C 底层 + 分配器理解 | L0 |
| **P2** | epoll ET + 线程池高并发服务器 | 用户态网络 + 线程模型 | L1 |
| **P3** | UDP 组播行情接收器 + 分层丢包定位 | 网络调优 + 排障方法论 | L3 |
| **P4** | SPSC 无锁 ring（含 padding/批量/内存序对比） | cache + 内存序 + 无锁 | L4 |
| **P5** | 三进程交易链路 + PTP 分段延迟测量 | 系统整合 + 测量 | L5 |

> 面试时**能说出每个项目优化前后的具体数字和归因**，比项目数量重要得多。

---

## 十二、自欺检查清单

- [ ] 我说「我做了无锁队列」——**我用两种 memory_order 对比过吗？在弱序架构上验证过吗？**
- [ ] 我说「我调优了」——**我有优化前后的 p99 数字吗？还是只感觉变快了？**
- [ ] 我说「我懂 NUMA」——**我跑过 `numactl` 前后对比吗？**
- [ ] 我说「我懂 LOB」——**我能写出撮合循环吗？处理过撤单吗？**
- [ ] 我说「我懂内核网络」——**我能画出完整收包路径并指出延迟在哪一段吗？**
- [ ] 我说「低延迟」——**我测的是 p50 还是 p999？max 是多少？抖动来源定位了吗？**

---

## 相关

- 阅读顺序 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
- 书目章节裁剪 → [READING-LIST.md](../READING-LIST.md)
- 无锁与内存布局 → [chapter-07](./chapter-07-lockless-data-structures-memory-layout/README.md)
- 延迟测量 → [chapter-09](./chapter-09-latency-measurement-benchmarking/README.md)
- OS 调优 → [chapter-05](./chapter-05-os-kernel-tuning/README.md)
- 实战五步 → [chapter-01 §1.8](./chapter-01-hft-fundamentals-ecosystem/1.8-实战启动建议.md)
- 市场微观结构 → [19-markets-microstructure](../19-markets-microstructure/)
