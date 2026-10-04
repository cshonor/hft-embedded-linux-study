# OpenOnload / RDMA 与 DPDK 对比

> **02-Advanced-Book** · 《Linux 高性能网络详解》配套 · **选读**

> **本篇分工：** [note-XDP与DPDK对照](./note-XDP与DPDK对照.md) 讲**内核内的**渐进加速；
> 本篇讲**彻底绕开内核**的三条路线（OpenOnload / DPDK / RDMA）各自的代价，
> 以及 HFT 里"行情通道 vs 订单通道"为什么常常选不同答案。

---

## 一、路线概览

| 路线 | 典型产品/技术 | API 语义 | 旁路程度 | 端到端延迟量级 |
|------|--------------|----------|----------|----------------|
| 标准内核栈 | UNP socket + Rosen 内核栈 | `socket` / `epoll` | 无 | 10–50μs（默认）→ 2–5μs（NAPI defer） |
| 内核旁路 + Socket 兼容 | **OpenOnload** | 保留 BSD socket API | 部分旁路 | ~1–5μs |
| 内 hook + 用户态收包 | **AF_XDP（zero-copy）** | ring + UMEM | 按流旁路 | ~0.5–2μs |
| 用户态旁路 | **DPDK** | `rte_eth_*` / mbuf | 完全旁路 | ~0.3–1μs |
| 硬件 RDMA | **RoCE / InfiniBand** | ibverbs / rdma_cm | 内核/用户态可选 | ~0.5–2μs（单边 RTT ~1μs 内） |

延迟量级只是**同一台机器上可互相比较的相对刻度**，不是绝对值：
取决于网卡、CPU 代际、内核版本、是否 isolcpus、包大小。
**真正可信的数字只有你自己按
[延迟测量方法](../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)
实测出来的那一组。**

---

## 二、RDMA：传输层下沉到硬件

### 2.1 为什么快——三件事都是网卡做的

| 传统路径的开销 | RDMA 怎么消掉 |
|---|---|
| 内核协议栈处理 | **传输层在网卡 ASIC 里实现**（可靠、重传、拥塞控制全硬件） |
| 数据拷贝（socket buffer → 应用） | **零拷贝**：网卡 DMA 直接读写应用注册好的内存 |
| 内核↔用户态切换 | **内核旁路**：`ibv_post_send` 就是往门铃寄存器写一次 MMIO |

### 2.2 Verbs 语义：通信模型的根本不同

这是从 socket 转 RDMA 最大的思维转变——**不是"收发消息"，而是"操作对端内存"**：

| 操作 | 语义 | 对端 CPU 参与 | 典型用途 |
|---|---|---|---|
| `SEND` / `RECV` | 双边：对端要先挂接收缓冲 | 要 | 消息传递（最接近 socket 思维） |
| `RDMA READ` | 单边：直接读对端内存 | **不参与** | 拉行情快照、读共享状态 |
| `RDMA WRITE` | 单边：直接写对端内存 | **不参与** | 推送订单/行情，最低延迟路径 |
| `ATOMIC` (CAS/FAA) | 单边原子操作 | **不参与** | 无锁序列号、心跳计数 |

**单边操作的杀伤力**：对端 CPU 全程不知情、不中断、不过任何软件——
WRITE 落地后数据就在对端内存里了。这是 ~1μs RTT 的来源，
也是 DPDK 做不到的（DPDK 对端还是要软件收包解析）。

代价是**编程模型全变**：没有"连接上的字节流"，只有：

```
QP (Queue Pair)  ── 一对工作队列：SQ（发）+ RQ（收）
CQ (Completion)  ── 完成队列，poll 它知道哪笔操作落地了
MR (Memory Region) ── 注册内存：pin 住 + 拿到 lkey/rkey
```

### 2.3 代码骨架（RC + 单边 WRITE）

```c
/* 1. 注册内存 —— 之后的所有操作都基于 lkey/rkey，不是裸指针 */
struct ibv_mr *mr = ibv_reg_mr(pd, buf, len,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);

/* 2. 建 RC QP（Reliable Connected，HFT 最常用的传输类型） */
struct ibv_qp_init_attr qp_attr = {
    .send_cq = cq, .recv_cq = cq,
    .qp_type = IBV_QPT_RC,
    .cap = { .max_send_wr = 128, .max_recv_wr = 128,
             .max_send_sge = 1, .max_recv_sge = 1 },
};
struct ibv_qp *qp = ibv_create_qp(pd, &qp_attr);
/* 之后要经历 RESET→INIT→RTR→RTS 状态机，并交换对端 QPN/GID（rdma_cm 或手工） */

/* 3. 单边 WRITE —— 热路径上就这么多 */
struct ibv_sge sge = {
    .addr = (uintptr_t)payload, .length = len, .lkey = mr->lkey };
struct ibv_send_wr wr = {
    .opcode = IBV_WR_RDMA_WRITE,       /* 对端 CPU 无感 */
    .send_flags = IBV_SEND_SIGNALED,
    .wr.rdma = { .remote_addr = peer_addr, .rkey = peer_rkey },
    .sg_list = &sge, .num_sge = 1,
};
struct ibv_send_wr *bad;
ibv_post_send(qp, &wr, &bad);          /* ≈ 一次 MMIO 门铃写 */

/* 4. poll 完成队列（和 DPDK 一样，无中断轮询） */
struct ibv_wc wc;
while (ibv_poll_cq(cq, 1, &wc) == 0) { /* spin */ }
```

对比 DPDK 收包循环：同样是无中断轮询，但 RDMA **连"包"的概念都没有**——
没有解析、没有协议头处理，写完即完成。

### 2.4 RoCE 的现实坑：无损以太网

RDMA 硬件传输层**假设底层不丢包**（重传代价极高）。InfiniBand 天然无损；
RoCE（RDMA over Converged Ethernet）跑在普通以太网上，要靠数据中心交换技术硬造出"无损"：

| 机制 | 作用 | 坑 |
|---|---|---|
| **PFC**（802.1Qbb 优先级流控） | 拥塞时按优先级 PAUSE，不丢包 | PAUSE 风暴会**殃及同优先级无辜流量**；配置错就是全网抖动 |
| **ECN + DCQCN** | 拥塞早期打标记，主动降速 | 网卡/交换机参数要配套调，否则要么饿要么堵 |
| **RoCEv2**（UDP/IPv4 封装） | 可路由、能过三层 | GID/MTU（要 9000 jumbo）配置琐碎 |

**HFT 的现实结论**：托管机房内、交换机归自己/券商管的二层小环境，RoCE 可行且值得；
跨机房、经过不控制的网络，RoCE 的无损前提就塌了——那种场景老老实实走 TCP/UDP。

---

## 三、OpenOnload：把旁路藏在 socket 后面

### 3.1 架构与生效条件

```
应用（不改代码，还是 socket()/send()/recv()）
   │  LD_PRELOAD=libonload.so  /  onload ./app
   ▼
Onload 用户态库 ── 符合加速规则的 socket → 直接在用户态处理 TCP/UDP
   │                    （网卡：Solarflare/Xilinx，后被 AMD/NVIDIA 收购线）
   └── 不符合规则的（如非加速接口、某些 sockopt）→ 回退内核栈
```

**关键事实：它只加速"它认识的"socket。** 判断有没有真被加速：

```bash
onload_stackdump lots | grep -A5 <你的socket>   # 看在不在加速栈上
```

### 3.2 定位：订单通道，不是行情通道

| | OpenOnload | DPDK |
|---|---|---|
| API | **BSD socket，老代码零改动** | mbuf/rte_eth，全部重写 |
| 协议 | TCP/UDP 都有状态机 | **只有包，没有 TCP** |
| 可见性 | 黑盒，出问题靠厂商工具 | 全在你手里，xstats 随便看 |
| 延迟 | ~1–5μs | ~0.3–1μs |
| 网卡绑定 | Solarflare/Xilinx 系 | 主流 PMD 均可 |

HFT 里它的典型位置是**订单通道**：交易所订单接口基本是 TCP（要状态机、要重传语义），
用 DPDK 自己实现 TCP 不现实，用内核栈又慢——Onload 恰好卡在这个缝里。
行情通道（UDP 组播、无状态）则直接 DPDK 拿最后那点确定性。

底层还有一层 **EF_VI**（Onload 的裸 API，绕过 socket 语义），延迟更接近 DPDK，
但同样绑死厂商网卡——选型时把它当作"DPDK 的厂商限定版"来比较。

---

## 四、HFT 选型决策

```
流量是什么？
├── UDP 组播行情（无状态、要确定性）──────────→ DPDK
├── TCP 订单/回报（要状态机、要快）──┬ 自有网卡选型权 → OpenOnload / EF_VI
│                                  └ 网卡不受限     → 内核栈精调（NAPI defer）
├── 托管机房内、网络可控、纳秒必争 ──────────→ RDMA/RoCE（单边 WRITE）
├── 渐进提速、不能动基础设施 ───────────────→ AF_XDP（zc）→ 见对照篇
└── 跨机房/互联网 ─────────────────────────→ 内核栈（RoCE 无损前提不成立）
```

**组合是常态**：同一台机器行情口 DPDK、订单口 Onload、管理口内核栈+XDP，
三张网卡三条路线各干各的——互不冲突，因为旁路的单位是"网卡/流"，不是"机器"。

---

## 五、官方参考

- OpenOnload：https://www.openonload.org/
- RDMA 规范：https://www.infinibandta.org/
- Linux RDMA 子系统：https://www.kernel.org/doc/html/latest/infiniband/
- man：`ibv_post_send(3)`、`ibv_reg_mr(3)`、`rdma_cm(7)`

## 相关章节

- [note-DPDK实体书递进](../../01-Intro-Book/notes/note-DPDK实体书递进.md) — ② 本书与 DPDK/RDMA/XDP 全书地图
- 上一梯度：[01-Intro chapter-05 组播行情接入](../../01-Intro-Book/notes/chapter-05-组播行情接入.md)
- XDP/AF_XDP 对照：[note-XDP与DPDK对照](./note-XDP与DPDK对照.md)
- DPDK 侧零拷贝原理：[chapter-04-零拷贝与用户态旁路](../../01-Intro-Book/notes/chapter-04-零拷贝与用户态旁路.md)
- 跨模块：[README 跨模块对照](../../../README.md)
