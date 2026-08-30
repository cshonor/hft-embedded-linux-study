# 03 — UMEM 布局与 copy / zero-copy 的微观差异

> **对应 Rosen:** 无（AF_XDP 4.18+ 才存在）
> **内核源码路径:** `Documentation/networking/af_xdp.rst`、`net/xdp/xsk.c`、`net/xdp/xsk_queue.h`

## 文档概述

[01-af-xdp](./01-af-xdp.md) 讲了创建流程和"两种模式"，但没有回答两个关键问题：
**UMEM 到底怎么布局**、**copy 模式比 zero-copy 多付出什么**。
这两点决定了 AF_XDP 值不值得用——答案是：只有 zero-copy 才值得。

---

## 核心内容

### UMEM：一块被用户态和网卡共享的内存

UMEM 是用户态分配的一块连续内存，划成等长的 **frame**，网卡和用户态都直接读写它。

```
UMEM (用户态 mmap，建议 hugepage 或 posix_memalign)
┌──────────┬──────────┬──────────┬──────────┬─────
│ frame 0  │ frame 1  │ frame 2  │ frame 3  │ ...
└──────────┴──────────┴──────────┴──────────┴─────

单个 frame 内部（frame_size 通常 2048 或 4096）：
┌────────────┬─────────────────┬───────────┐
│  headroom  │   packet data   │ tailroom  │
│  (256 B)   │                 │           │
└────────────┴─────────────────┴───────────┘
      ↑ 供 XDP 程序 bpf_xdp_adjust_head() 用（加/去封装头）
```

**关键：`xdp_desc.addr` 是 UMEM 区域内的字节偏移，不是指针。**

```c
struct xdp_desc {
    __u64 addr;     /* 相对 UMEM 起始的偏移 */
    __u32 len;      /* 包长度 */
    __u32 options;
};

/* 由偏移拿到数据指针 */
void *pkt = xsk_umem__get_data(umem_area, desc.addr);
```

注册参数（`setsockopt(XDP_UMEM_REG)`）：

| 字段 | 说明 | 典型值 |
|------|------|--------|
| `addr` | UMEM 起始地址 | mmap 返回值 |
| `len` | UMEM 总大小 | frame_size × frame_count |
| `frame_size` | 每个 frame 长度 | 2048（小包）/ 4096（含 jumbo） |
| `frame_headroom` | 每 frame 前置保留 | 256（`XDP_PACKET_HEADROOM`） |

> `addr` 必须按 frame_size 对齐；建议 UMEM 落在 hugepage 上，减少 TLB miss。

---

### 四个 ring 的生产者 / 消费者关系

```
                 ┌─────────────── 内核态 ───────────────┐
                 │                                      │
   FILL ring  ──→│  空闲 frame 地址池（用户填，内核取）  │
                 │            ↓                          │
                 │      网卡 DMA 写入 frame              │
                 │            ↓                          │
   RX ring    ←──│  已收包描述符（addr + len）            │
                 │                                      │
   TX ring    ──→│  待发包描述符（用户填，内核取）         │
                 │            ↓                          │
  COMPLETION  ←──│  已发完的 frame（可复用）              │
                 │                                      │
                 └──────────────────────────────────────┘
```

| Ring | 方向 | 作用 | 空了/满了会怎样 |
|------|------|------|----------------|
| FILL | 用户 → 内核 | 提供空闲 frame 给网卡写 | **空 → 收包丢包** (`rx_dropped`) |
| RX | 内核 → 用户 | 交付已收包 | 用户读慢 → 队列积压，最终丢 |
| TX | 用户 → 内核 | 提交待发包 | — |
| COMPLETION | 内核 → 用户 | 归还已发送 frame | 用户不回收 → 无空闲 frame |

**HFT 最重要的运维点：FILL ring 水位。**
收包路径是"内核从 FILL ring 拿 frame → DMA 写入 → 放回 RX ring"，
用户态消费 RX ring 后必须**把 frame 地址重新填回 FILL ring**，否则水位耗尽就开始丢包。

```c
/* 典型主循环骨架（libbpf xsk.h） */
uint32_t idx_rx, idx_fill, rcvd;

rcvd = xsk_ring_cons__peek(&rx, BATCH, &idx_rx);
if (rcvd > 0) {
    /* 1. 预留同等数量的 FILL 槽位，把用完的 frame 还回去 */
    xsk_ring_prod__reserve(&fill, rcvd, &idx_fill);
    for (i = 0; i < rcvd; i++) {
        uint64_t addr = xsk_ring_cons__rx_desc(&rx, idx_rx + i)->addr;
        /* ... 处理包 ... */
        *xsk_ring_prod__fill_addr(&fill, idx_fill + i) = addr;  /* 归还 */
    }
    xsk_ring_prod__submit(&fill, rcvd);
    xsk_ring_cons__release(&rx, rcvd);
}
```

**建议：** FILL ring 大小 ≥ 2048，运行时水位保持 > 50%。
批处理（一次还一批）能摊薄 ring 操作开销，但要权衡批量带来的延迟。

---

### copy vs zero-copy：差的到底是哪一步

这是本笔记的核心。两条路径对比：

```
【copy 模式】—— 名字叫 XDP，其实没省掉关键开销
  NIC DMA → page_pool page
      → 驱动分配 sk_buff            ← 1. 分配开销 ~100ns
      → NAPI poll → XDP_REDIRECT
      → 内核把 payload memcpy 到 UMEM frame   ← 2. 拷贝开销 20~200ns（随包长）
      → RX ring 描述符
      → 用户态读取

【zero-copy 模式】—— 真旁路
  NIC DMA 直接写进 UMEM frame（page_pool 与 UMEM 共享映射）
      → 驱动只填一个 xdp_desc       ← 无 sk_buff、无 memcpy
      → RX ring 描述符
      → 用户态读取（就是网卡刚写过的那块内存）
```

| 开销项 | copy | zero-copy | 差值 |
|--------|------|-----------|------|
| sk_buff 分配 | 有 | **无** | ~100 ns |
| payload memcpy | 有 | **无** | 20–200 ns（随包长） |
| 缓存污染 | 两次触碰数据 | 一次 | 影响后续访问 |
| 典型收包延迟 | 1.5–3 μs | 0.5–1 μs | **2–3x** |
| 典型吞吐/core | 3–6 Mpps | 10–24 Mpps | **3–4x** |

**结论：** copy 模式省掉的只是协议栈遍历，**仍然分配 sk_buff、仍然 memcpy**。
它相对 `recvmsg()` 的优势有限，却要付出独占队列 + BPF 开发的代价。
所以 [01-af-xdp](./01-af-xdp.md) 说"零拷贝是唯一理由"——用之前先用 `XDP_ZEROCOPY` 试，
驱动不支持就老实走内核栈 + busy poll，别用 copy 模式。

```bash
# 校验你的驱动是否真支持 zero-copy（绑定时带 XDP_ZEROCOPY flag）
# 绑定失败或退化为 copy 模式，说明驱动/队列配置不支持
xdp-loader load -m native eth0 prog.o
```

---

### 与 DPDK 的结构对照

| 维度 | AF_XDP zero-copy | DPDK |
|------|-----------------|------|
| Rx 缓冲区 | UMEM frame（page_pool 映射，内核分配） | rte_mbuf（hugepage mempool，用户分配） |
| 描述符 | `xdp_desc{addr,len}` 数组 | `rte_mbuf*` 指针数组 |
| 收包 ring | RX ring + FILL ring（两套） | 硬件 Rx descriptor ring（一套） |
| buffer 归还 | 用户显式填 FILL ring | 驱动复用 desc，用户 free mbuf |
| 驱动 | **内核驱动** + XDP 程序 | 用户态 PMD（UIO/VFIO） |
| 网卡独占 | 仅该队列 | 整张网卡 |
| 内核功能共存 | ✅ 其他队列走协议栈 | ❌ 网卡被接管 |
| 首次 touching | 内核已碰过（填 desc） | 用户态独占，缓存更干净 |

**结构性差异：** AF_XDP 的 buffer 所有权由内核和用户态**来回交接**（FILL/RX 两套 ring），
DPDK 是用户态**独占** mbuf 池。这套交接带来了额外的 ring 操作，
是 AF_XDP 在极限延迟上略逊 DPDK 的主要原因。

---

## HFT 要点

- **先验证 zero-copy 能用**，驱动不支持就别用 AF_XDP——copy 模式不划算
- `xdp_desc.addr` 是**偏移不是指针**，用 `xsk_umem__get_data()` 转换
- **FILL ring 水位是生命线**：`xdp-stat` 里 `rx_dropped` 涨 = 你归还 frame 太慢
- UMEM 放 hugepage 上，frame_size 按最大包长选（行情包通常小，2048 够）
- headroom 留 256 B，方便 XDP 程序做 `bpf_xdp_adjust_head()`
- AF_XDP 只接管**指定队列**，其他队列仍走内核栈——这是它相对 DPDK 最大的工程优势

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| 用户态收包 | 只有 socket + recvmsg | AF_XDP 零拷贝，绕过协议栈 |
| Rx buffer 分配 | 每次 alloc_page | page_pool 复用，UMEM 直接映射 |
| 内核/用户交接 | 一次拷贝（recvmsg） | 零拷贝 + FILL/RX ring 所有权交接 |
| 队列粒度旁路 | 无此概念 | AF_XDP 可按队列旁路，与内核栈共存 |
