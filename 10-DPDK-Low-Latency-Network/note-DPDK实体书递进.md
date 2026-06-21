# DPDK 实体书 · 《深入浅出 DPDK》→《Linux 高性能网络详解》

> **10-DPDK** · 实体书补充 · **场景触发再读**  
> 本仓库 `10` 文件夹主线仍是 [DPDK 官方文档](./README.md)；两本书帮你**建立认知 → 挖深度**，与仓库 chapter-01–05 对照阅读。

---

## 两本书 · 递进关系

| 顺序 | 书名 | 角色 | 你要带走什么 |
|------|------|------|--------------|
| **①** | **《深入浅出 DPDK》** | 建立认知 | 绕开内核协议栈、用户态 PMD 接管网卡、EAL/mbuf/mempool 直觉 |
| **②** | **《Linux 高性能网络详解》** | 挖深度 | DPDK 与 **RDMA、XDP** 等进阶路线；微秒级延迟从哪来；何时上 RDMA |

**读法：** 先 ① 搞懂「DPDK 在干什么」→ 再 ② 串起「Linux 高性能网络全家桶」与选型。

---

## ① 《深入浅出 DPDK》— 快速建立核心思路

**对应 HFT 场景：** 高频发单 / 收行情时 **抠网络延迟** — 想知道「为什么不走内核 socket 还能更快」。

| 主题 | 与量化系统的对应 |
|------|------------------|
| **绕过内核协议栈** | tick 热路径少 syscall、少 sk_buff 拷贝 |
| **用户态驱动（PMD）** | 轮询收包、绑核、与策略线程同 NUMA |
| **大页 / mempool / mbuf** | 预分配、热路径零 malloc（衔接 `01` CSAPP Ch6/9） |

**与本仓库笔记对照：**

| 书（概念） | 本仓库 |
|------------|--------|
| DPDK 架构、EAL | [chapter-01-DPDK架构与EAL](./chapter-01-DPDK架构与EAL.md) |
| mbuf、mempool | [chapter-02-mbuf与内存池](./chapter-02-mbuf与内存池.md) |
| PMD、poll mode | [chapter-03-PMD与轮询模式](./chapter-03-PMD与轮询模式.md) |
| 旁路、零拷贝 | [chapter-04-零拷贝与用户态旁路](./chapter-04-零拷贝与用户态旁路.md) |
| 组播行情 | [chapter-05-组播行情接入](./chapter-05-组播行情接入.md) |

---

## ② 《Linux 高性能网络详解》— 进阶与选型

**在第一本基础上：**

- 把 **DPDK、RDMA、XDP** 等路线放在同一张地图里对比
- 解释 **DPDK 为何能把延迟压到微秒级**（旁路 + 轮询 + 预分配 + 绑核，与 `02` SysPerf 度量对齐）
- 说明 **什么时候该用 RDMA** 做更极致优化（共置、托管、纳秒级共址 — 见 [note-openonload-rdma对比](./note-openonload-rdma对比.md)）

| 技术 | 与 DPDK 关系 | HFT 典型场景 |
|------|--------------|--------------|
| **DPDK** | 用户态完全旁路 | UDP 组播行情、极致 tick 处理 |
| **XDP / tc-BPF** | 内核最早点丢/改包 | 对比 DPDK 的「半旁路」；见 [03-BPF note-XDP](../03-BPF-Performance-Tools/note-XDP与tc-BPF.md) |
| **RDMA / RoCE** | 硬件 offload、远端内存 | 共置机房、极低延迟通道 |
| **OpenOnload** | 保留 socket API 的内核旁路 | TCP 发单、迁移成本较低 |

---

## 何时读 · 不要过早

**建议触发条件（满足后再开 ①）：**

```
✅ 01 CSAPP 地基（尤其 Ch6 缓存、Ch10–11 网络）
✅ 02 SysPerf 方法论 — 会用 perf/BPF 做延迟分解
✅ 07 → 08 → 09 走完 — 知道内核栈收发包路径（对照「绕过了什么」）
✅ perf 已能定位：网络收发是瓶颈（或 softirq / 网卡队列饱和）
```

**还不急的情况：**

- 刚学完 `01`/`02`，还没摸过 `recvfrom`/`epoll` 与内核 NAPI 路径
- 延迟瓶颈仍在策略计算、锁、cache miss — **先优化应用，再上 DPDK**

**读完 ① ② 之后：**

- 回到 [11-HFT-Low-Latency-Practice](../11-HFT-Low-Latency-Practice/) ch06/ch08 — 把技术落到量化系统
- 用 [03-BPF](../03-BPF-Performance-Tools/) + `02` SysPerf Ch10 在生产上**验证**旁路收益

---

## 推荐阅读顺序（实体书 + 本仓库）

```
09 Rosen（内核栈：搞懂「要绕过谁」）
    ↓
① 《深入浅出 DPDK》  ∥  10 官方 doc + chapter-01–04
    ↓
10 chapter-05 + code/mcast-minimal（组播落地）
    ↓
② 《Linux 高性能网络详解》  ∥  note-openonload-rdma对比 + 03-BPF XDP note
    ↓
11 HFT Practice · ch06 低延迟网络
```

---

## 相关章节

- [README · 网络全链路](./README.md)
- [OUTLINE.md](./OUTLINE.md)
- [note-openonload-rdma对比.md](./note-openonload-rdma对比.md)
- [CROSS-MODULE-GUIDE §二](../CROSS-MODULE-GUIDE.md#二内核网络栈-vs-用户态旁路)
