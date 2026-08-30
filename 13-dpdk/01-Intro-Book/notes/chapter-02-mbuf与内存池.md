# Ch 2 mbuf 与内存池 · mbuf & Mempool

> **01-Intro-Book** · 官方 Programmer's Guide · **精读**

> **实体书：** [chapter-06-PCIe与包处理IO](../chapter-06-pcie-packet-io/) §6 精讲 mbuf/mempool 布局与 Core Cache；先读 [chapter-02-Cache与内存](../chapter-02-cache-and-memory/)（大页、NUMA、Cache 对齐）。

> **本篇分工：** 实体书 §6 讲**结构与理论**（`rte_mbuf` 长什么样、mempool 两级结构怎么做到无锁）。
> 本篇是**实验向**：参数怎么选、跑起来能观测到什么、出问题怎么定位。两篇对着看。

> **实验：** [code/mcast-minimal/](../code/mcast-minimal/) —— `main.c` 里那一行
> `rte_pktmbuf_pool_create()` 就是本篇的全部落点。

---

## 一、热路径上根本没有 alloc

这是最容易被搞混的一点，先说清楚。

```
初始化：  rte_eth_rx_queue_setup(port, q, RX_DESC, sid, NULL, mbuf_pool)
              └─ PMD 从 pool 批量取 mbuf，把缓冲区地址填进 RX 描述符环

收包时：  网卡 DMA 直接写描述符指向的 mbuf 数据区 → 置 DD 位
          rte_eth_rx_burst()
              ├─ 看到 DD 位 → 把 mbuf 指针交给应用
              └─ 顺手再从 pool 取新 mbuf 补回描述符环（攒够阈值才做）

应用：    parse → ... → rte_pktmbuf_free(m)   ← 应用只负责还
```

所以 **RX 路径上一行 `rte_pktmbuf_alloc()` 都不用写**。

免费的代价是：pool 里的 mbuf 大部分时间**在网卡手里**（已填进描述符、等着被 DMA 写），
而不是在你手里。这直接决定了 pool 大小怎么算——见第四节。

顺带一个推论：既然 refill 是 `rx_burst` 内部做的，
**pool 耗尽首先表现为 `rx_nombuf` 增长，而不是 `rte_pktmbuf_alloc()` 返回 NULL**。

---

## 二、把 mbuf 打出来看

结构定义不用背，直接跑：

```c
printf("sizeof(rte_mbuf)          = %zu\n", sizeof(struct rte_mbuf));
printf("RTE_PKTMBUF_HEADROOM      = %d\n",  (int)RTE_PKTMBUF_HEADROOM);
printf("RTE_MBUF_DEFAULT_DATAROOM = %d\n",  (int)RTE_MBUF_DEFAULT_DATAROOM);
printf("RTE_MBUF_DEFAULT_BUF_SIZE = %d\n",  (int)RTE_MBUF_DEFAULT_BUF_SIZE);

/* 单个包的完整信息 */
rte_pktmbuf_dump(stdout, m, m->data_len);

/* pool 水位 —— 诊断 rx_nombuf 的关键 */
printf("in_use=%u avail=%u\n",
       rte_mempool_in_use_count(pool),
       rte_mempool_avail_count(pool));
```

典型输出（x86-64，DPDK 22.11）：

| 量 | 值 | 含义 |
|---|---|---|
| `sizeof(struct rte_mbuf)` | 128 | 恰好 2 条 Cache Line，被 `__rte_cache_aligned` 强制对齐 |
| `RTE_PKTMBUF_HEADROOM` | 128 | `buf_addr` 到 `data_off` 的预留 |
| `RTE_MBUF_DEFAULT_DATAROOM` | 2048 | 单 mbuf 能装的数据量 |
| `RTE_MBUF_DEFAULT_BUF_SIZE` | 2176 | 上面两者之和 |

**dataroom 为什么是 2048？** 不是凑整，是让**一个标准帧完整落进单个 mbuf**——
不产生链式 mbuf，热路径就不用走 `nb_segs` 循环。

⚠ **jumbo 帧会打破这个假设。** 1500 MTU 毫无问题；9K jumbo 帧在 2048 dataroom 下
必然分段，必须开 `RTE_ETH_RX_OFFLOAD_SCATTER`，否则网卡直接截断或丢弃。
行情包通常远小于 MTU，遇不到；但接交易所的 **TCP 补单通道**时可能踩到。

---

## 三、mempool 六个参数怎么填

```c
struct rte_mempool *p = rte_pktmbuf_pool_create(
    "MBUF_POOL",   /* name          */
    n,             /* 元素个数       */
    cache_size,    /* per-lcore cache */
    0,             /* priv_size     */
    RTE_MBUF_DEFAULT_BUF_SIZE,
    socket_id);
```

| 参数 | 建议 | 理由 / 坑 |
|---|---|---|
| `n` | **取 2^k − 1**，如 8191 | mempool 底层是 `rte_ring`，容量必须是 2^k；内部按 `align32pow2(n+1)` 分配。填 8192 → ring 按 16384 槽位分配，**多浪费一份 ring 索引数组（16K × 8B ≈ 128KB）**且毫无收益。这就是 `mcast-minimal` 里写 8191 而不是 8192 的唯一原因 |
| `cache_size` | 250（热核）/ 0（多核共享且内存紧） | 上限 `RTE_MEMPOOL_CACHE_MAX_SIZE` = 512。太大 → 每核囤货，总内存翻倍、冷启动局部性差；太小 → 频繁回落全局 ring，CAS 争用回来 |
| `priv_size` | 0，除非有明确用途 | 在 mbuf 后追加应用私有区（放硬件时间戳、解析中间结果很香），但会改变对象大小，改了要重算内存占用 |
| `data_room_size` | 默认 2176 | 只在确认有 jumbo 时才调大，**调大 = 每个 mbuf 都变贵** |
| `socket_id` | ★ **`rte_eth_dev_socket_id(port_id)`** | 见下 |

### socket_id：HFT 最容易填错的一个

```c
rte_socket_id();             /* 调用者所在 NUMA 节点 */
rte_eth_dev_socket_id(pid);  /* 网卡所在 NUMA 节点   */
```

`mcast-minimal` 里填的是 `rte_socket_id()`（单路机器上两者相同，无所谓）。
**双路机器上这是实打实的延迟差**：

- pool 建在**网卡所在节点** → DMA 写本地内存，走本地 PCIe root
- pool 建在**对端节点** → 每写一个包都要跨 UPI/QPI，还得在远端 cache 一致性域里来回

正确写法：

```c
int sid = rte_eth_dev_socket_id(port_id);
struct rte_mempool *p = rte_pktmbuf_pool_create("MBUF_POOL", NB_MBUF,
                                                CACHE, 0,
                                                RTE_MBUF_DEFAULT_BUF_SIZE, sid);
rte_eth_rx_queue_setup(port_id, q, RX_DESC, sid, NULL, p);
```

验证：`numactl -H` 看节点拓扑，`lstopo` 看网卡挂在哪个 socket 下。

---

## 四、pool 大小怎么算（配 `rx_nombuf` 一起看）

在途 mbuf 有四个去处，**漏掉任何一项都会导致偶发 `rx_nombuf`**：

```
n  ≥   Σ RX_DESC        （各队列，网卡手里）
    +  Σ TX_DESC        （各队列，发送在途）
    +  各 lcore × BURST_SIZE  （刚从 rx_burst 拿出来还没处理）
    +  应用自己持有着的        （压进 ring 排队、攒 batch 等）
    +  余量
```

`mcast-minimal` 是单队列：4096(RX) + 4096(TX) + 32(burst) ≈ 8224，
取 8191 其实**刚好卡线**。真实系统请按上式重算，别照抄示例。

### 观测手段

| 工具 | 看什么 |
|---|---|
| `rte_eth_stats_get()` → `rx_nombuf` | pool 耗尽次数（**用户态背压**） |
| `rte_eth_stats_get()` → `imissed` | 网卡侧收不进来（**描述符/PCIe 背压**） |
| `rte_mempool_in_use_count()` | 实时水位，跑起来打点看趋势 |
| `dpdk-procinfo -- --stats` | 外部观测，不用改程序 |

**判据：`rx_nombuf` 涨而 `imissed` 不涨 → 是你（用户态）慢了，不是网卡慢了。**
反过来 `imissed` 涨 → 描述符环太小或 PCIe 跟不上，加大 `RX_RING_SIZE`。

---

## 五、三个必踩的坑

**1. 忘了 `rte_pktmbuf_free()`**

内核里 `sk_buff` 靠引用计数自动回收，DPDK 完全靠你手动还。
症状：跑几秒或几分钟后 `rx_nombuf` 开始涨，之后丢包率稳步上升。
单看代码很难发现，靠 `rte_mempool_in_use_count()` 打点——**水位单调上升就是漏 free**。

**2. free 了两次**

比漏 free 更糟：同一个 mbuf 被塞回 pool 两次，之后两个"消费者"拿到同一块内存，
表现为**完全随机、无法复现的诡异崩溃**。
典型触发：异常分支里已经 free 了，外层循环又 free 一次；
或把同一个 mbuf 指针既交给 TX 又自己 free。

**3. 多核共 pool 但 `cache_size` 乱填**

多核共享是安全的（全局 ring 是 MP/MC），但 `cache_size` 让每个 lcore 私囤一批，
总需求变成 `n + Σ cache`。核多的时候这两项能差出一个数量级。

---

## 六、与 sk_buff 对照（行为维度）

字段级对照见实体书 §6，这里只列**跑起来才看得出的差别**：

| 维度 | `rte_mbuf` | `sk_buff` |
|---|---|---|
| 分配时机 | 初始化 / refill 时批量预分配，**热路径零 alloc** | 每包 `alloc_skb()` |
| 单次分配成本 | cache 命中 ~20–30 cycles；miss 回落全局 ring 跳到 ~200+ | slab 分配，~100ns 量级且含 atomic |
| 释放 | **必须手动** `rte_pktmbuf_free()` | 引用计数归零自动回收 |
| 回收器 | per-lcore cache（无锁）+ 全局 ring | slab + per-CPU cache（仍有 atomic） |
| 多订阅者 | 无复制概念，用户态自己决定分不分 | 组播多 socket → **每 socket 一份 skb 复制** |
| 链式 | `nb_segs` + `next`，需开 scatter offload | 分片/非线性区，内核透明处理 |
| 忘记回收的后果 | pool 耗尽 → `rx_nombuf` → 直接丢包 | 内存泄漏，但不直接影响收包 |

一句话：**sk_buff 用便利换安全，mbuf 用纪律换确定性。**
DPDK 那几百纳秒的优势里，有一部分是"你承诺自己管内存"换来的。

---

## 七、调参清单

```c
/* 自己 alloc 的场景（如构造发包）：用 bulk 版 */
rte_pktmbuf_alloc_bulk(pool, bufs, n);  /* 比循环 alloc 快：省重复的 cache 指针判断 */

/* 观测水位，跑起来周期性打点 */
printf("in_use=%u avail=%u\n",
       rte_mempool_in_use_count(pool), rte_mempool_avail_count(pool));
```

- [ ] `n` 是 2^k − 1
- [ ] `socket_id` 用的是网卡节点，不是 `rte_socket_id()`
- [ ] 大小按「描述符 + burst + 应用持有 + 余量」算过
- [ ] jumbo 帧场景确认开了 scatter offload
- [ ] 每个 `rx_burst` 拿到的 mbuf 都有且仅有一次 `free`

---

## 相关章节

- 实体书：[section-6-Mbuf与Mempool.md](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md)（结构与理论，本篇的前提）
- 上一章：[chapter-02-Cache与内存.md](./chapter-02-Cache与内存.md) · [chapter-03-并行计算.md](./chapter-03-并行计算.md)
- 下一章：[chapter-03-PMD与轮询模式.md](./chapter-03-PMD与轮询模式.md)（`rx_burst` 内部的 refill 就发生在这里）
- 无锁 ring：[chapter-04-synchronization/](../chapter-04-synchronization/)
- 内核对照：[12-kernel-networking](../../../12-kernel-networking/) · 缓存：[02-CSAPP Ch6](../../../02-computer-systems/chapter-06-memory-hierarchy/)
- 实验：[code/mcast-minimal/](../code/mcast-minimal/)
