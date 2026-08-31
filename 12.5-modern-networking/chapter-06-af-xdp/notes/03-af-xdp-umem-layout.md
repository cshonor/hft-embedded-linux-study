# 03 — UMEM 布局与 copy / zero-copy 的微观差异

> **对应 Rosen:** 无（AF_XDP 4.18+ 才存在）
> **内核源码路径:** `Documentation/networking/af_xdp.rst`、`net/xdp/xsk.c`、`net/xdp/xdp_umem.c`、`net/xdp/xsk_buff_pool.c`、`net/xdp/xsk_queue.h`
> **内核版本:** 以 **v6.6** 为准，注册校验与布局公式均取自源码

## 文档概述

[01-af-xdp](./01-af-xdp.md) 讲了接口每一步的约束，
[02-af-xdp-lwn](./02-af-xdp-lwn.md) 讲了工程决策与延迟预算，
本篇回答剩下两个微观问题：**UMEM 到底怎么布局**、**copy 模式比 zero-copy 多付出什么**。
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

注册参数（`setsockopt(XDP_UMEM_REG)`，`include/uapi/linux/if_xdp.h:73`）：

| 字段 | 说明 | 典型值 |
|------|------|--------|
| `addr` | UMEM 起始地址 | mmap / posix_memalign 返回值 |
| `len` | UMEM 总大小 | chunk_size × chunk 数 |
| `chunk_size` | 每个 chunk 的长度 | 2048（小包）/ 4096（留 headroom 或大包） |
| `headroom` | 每 chunk 的**额外**前置保留 | 0 或 256 |
| `flags` | `XDP_UMEM_UNALIGNED_CHUNK_FLAG` | 0（对齐模式） |

### ⚠️ 注册时的四条硬校验（`xdp_umem_reg()`，`net/xdp/xdp_umem.c:151`）

| # | 校验（行号） | 违反的 errno |
|---|-------------|-------------|
| 1 | `chunk_size < 2048 \|\| chunk_size > PAGE_SIZE`（:160） | `-EINVAL` |
| 2 | 对齐模式下 `chunk_size` 不是 2 的幂（:173） | `-EINVAL` |
| 3 | `!PAGE_ALIGNED(addr)`（:176） | `-EINVAL` |
| 4 | `len % chunk_size != 0`（:196）或 `headroom >= chunk_size - 256`（:199） | `-EINVAL` |

**两条最容易被写错的：**

- **`addr` 的对齐要求是"页对齐"，不是"chunk 对齐"。**
  `malloc()` 返回的地址不保证页对齐 → **`-EINVAL`**。
  用 `mmap(NULL, len, ..., MAP_ANONYMOUS\|MAP_HUGETLB, ...)` 或 `posix_memalign(&p, 4096, len)`。
- **`chunk_size` 的下界是 2048、上界是一页（x86_64 上 4096）**，
  且必须是 2 的幂（对齐模式下）。所以实际只有 **2048 和 4096** 两个选择。
  **想用一个 9 KB 的 chunk 装 jumbo frame 是做不到的**，只能用 `XDP_USE_SG` 拆多描述符。

### UMEM 注册后内核做了三件事（`xdp_umem_reg()`，:214-224）

```c
err = xdp_umem_account_pages(umem);                        /* ① RLIMIT_MEMLOCK 记账 */
err = xdp_umem_pin_pages(umem, (unsigned long)addr);       /* ② 钉页 */
err = xdp_umem_addr_map(umem, umem->pgs, umem->npgs);      /* ③ vmap 建内核映射 */
```

| 步骤 | 源码 | 后果 |
|------|------|------|
| ① 记账 | `rlimit(RLIMIT_MEMLOCK)` 超限 → `-ENOBUFS`（:144）；有 `CAP_IPC_LOCK` 才跳过 | 容器/CI 里最常见的启动失败原因 |
| ② 钉页 | `pin_user_pages(..., FOLL_WRITE \| FOLL_LONGTERM, ...)`（:105） | **UMEM 常驻物理内存，永不换出** |
| ③ 映射 | `vmap(pages, nr_pages, VM_MAP, PAGE_KERNEL)`（:49） | 建立内核侧虚拟映射，供 copy 模式访问 |

> 建议 UMEM 落在 hugepage 上，减少 TLB miss——但注意 `FOLL_LONGTERM` 对
> 部分特殊内存类型（如某些设备私有内存）会直接失败。

### `frame_len`：chunk 里真正能装包的字节数

```c
/* net/xdp/xsk_buff_pool.c:84 */
pool->frame_len = umem->chunk_size - umem->headroom - XDP_PACKET_HEADROOM;
```

| chunk_size | headroom | `frame_len` | 装 1500B MTU（实际 1514B） | 装 9000B jumbo |
|-----------|----------|------------|--------------------------|---------------|
| 2048 | 0 | **1792** | ✅ | ❌ |
| 2048 | 256 | **1536** | ⚠️ 仅剩 18 B 余量，带 VLAN 就悬 | ❌ |
| 4096 | 0 | **3840** | ✅ | ❌ |
| 4096 | 256 | **3584** | ✅ | ❌ |

**超出 `frame_len` 且没开 `XDP_USE_SG` 的包会被丢，计入 `rx_dropped`**（`xsk_rcv_check()`，xsk.c:317）。

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
  NIC DMA → page_pool page（驱动自己的普通收包路径）
      → 驱动构造 xdp_buff（mem.type = MEM_TYPE_PAGE_*）
      → NAPI poll → XDP_REDIRECT
      → __xsk_rcv()：
            xsk_buff_alloc(pool)      ← 1. 从 UMEM 取一个 frame
            memcpy(frame, pkt, len)   ← 2. 整包复制
            xdp_return_buff(xdp)      ← 3. 把驱动的 page 还回 page_pool
      → RX ring 描述符
      → 用户态读取（副本）

【zero-copy 模式】—— 真旁路
  NIC DMA 直接写进 UMEM frame
  （驱动把硬件 Rx 描述符指向 UMEM，UMEM 由内核 xp_dma_map() 做 DMA 映射）
      → 驱动构造 xdp_buff（mem.type = MEM_TYPE_XSK_BUFF_POOL）
      → NAPI poll → XDP_REDIRECT
      → __xsk_rcv_zc()：取 addr、填 desc   ← 就这两步
      → RX ring 描述符
      → 用户态读取（就是网卡刚写过的那块内存）
```

| 开销项 | copy | zero-copy | 差值 |
|--------|------|-----------|------|
| UMEM frame 分配 | 有（`xsk_buff_alloc`） | **无** | 池空还会 `rx_dropped++` |
| payload memcpy | **有（整包）** | **无** | 20–200 ns（随包长） |
| `xdp_return_buff()` | 有 | **无** | 页回收路径 |
| 缓存污染 | 两次触碰数据 | 一次 | 影响后续访问 |
| 内存带宽 | 2× 包长 | 1× 包长 | 线速下的硬指标 |
| 典型收包延迟 | 1.5–3 μs | 0.5–1 μs | **2–3x** |
| 典型吞吐/core | 3–6 Mpps | 10–24 Mpps | **3–4x** |

> ### ⚠️ 两处需要澄清的常见说法
>
> **① copy 模式并不分配 `sk_buff`。**
> XDP 在 skb 分配之前运行（见 [chapter-01](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md)），
> 两种模式下都没有 skb。copy 模式多付的是"**从 UMEM 取 frame + 整包 memcpy + 归还驱动页**"，
> 不是 skb 分配。
>
> **② zero-copy 不用 page_pool。**
> UMEM 由内核自己 DMA 映射——`struct xsk_buff_pool` 有自己的 `dma_pages` 数组
> （`include/net/xsk_buff_pool.h:68`），由 `xp_dma_map()` 填充。
> **说"AF_XDP 基于 page_pool"只在 copy 模式下成立**（那条路径上驱动确实用 page_pool）。
> 详见 [chapter-04/01](../../chapter-04-page-pool/notes/01-page-pool.md)。
>
> 判定是**逐包**做的，依据包的来源：
> ```c
> /* net/xdp/xsk.c:348 */
> if (xdp->rxq->mem.type == MEM_TYPE_XSK_BUFF_POOL)
>         return xsk_rcv_zc(xs, xdp, len);   /* 零拷贝 */
> err = __xsk_rcv(xs, xdp, len);             /* copy */
> ```

**结论：** copy 模式省掉的只是协议栈遍历，**仍然分配 frame、仍然 memcpy**。
它相对 `recvmsg()` 的优势有限，却要付出独占队列 + BPF 开发的代价。
所以用之前先按 [01 篇](./01-af-xdp.md)第四节的办法验证——**bind 时加 `XDP_ZEROCOPY`，
不支持就让它明确返回 `-EOPNOTSUPP` 失败**，别接受静默降级。

```bash
# 校验你的驱动是否真支持 zero-copy（bind 之后查，不是 bind 之前猜）
#   C 代码：getsockopt(fd, SOL_XDP, XDP_OPTIONS, &opts, &len)
#           opts.flags & XDP_OPTIONS_ZEROCOPY

# 或者：不带 XDP_ZEROCOPY bind 一次看是否成功、再用 bpftool/net 确认模式
bpftool net show dev eth0
```

---

### 与 DPDK 的结构对照

| 维度 | AF_XDP zero-copy | DPDK |
|------|-----------------|------|
| Rx 缓冲区 | UMEM frame（用户分配，内核 `xp_dma_map()` 映射） | rte_mbuf（hugepage mempool，用户分配） |
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

- **先验证 zero-copy 能用**，驱动不支持就别用 AF_XDP——copy 模式不划算。
  验证靠 `getsockopt(XDP_OPTIONS)` 的 `XDP_OPTIONS_ZEROCOPY`，不靠 bind 是否成功
- `xdp_desc.addr` 是**偏移不是指针**，用 `xsk_umem__get_data()` 转换
- **FILL ring 水位是生命线**，但别看错计数器：
  - `rx_fill_ring_empty_descs` 涨 = 归还 frame 太慢（ZC 模式头号丢包源）
  - `rx_ring_full` 涨 = RX ring 满，用户态消费太慢
  - `rx_dropped` 涨而上面两个为 0 = **包比 `frame_len` 大**（MTU 不匹配）
- UMEM 放 hugepage 上；`chunk_size` 只有 2048 / 4096 可选（4K 页系统），
  按最大包长选——**要留 256 B headroom 就直接上 4096**，否则 2048 只剩 1536 B
- headroom 方便 XDP 程序做 `bpf_xdp_adjust_head()`，但**它是从 `frame_len` 里扣的**
- UMEM 注册会 `pin_user_pages(FOLL_LONGTERM)` 常驻内存并吃 `RLIMIT_MEMLOCK`，
  容器里失败先查 `ulimit -l`
- AF_XDP 只接管**指定队列**，其他队列仍走内核栈——这是它相对 DPDK 最大的工程优势

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| 用户态收包 | 只有 socket + recvmsg | AF_XDP 零拷贝，绕过协议栈 |
| Rx buffer 分配 | 每次 alloc_page | UMEM 一次性分配 + DMA 长期映射（驱动侧另用 page_pool） |
| 内核/用户交接 | 一次拷贝（recvmsg） | 零拷贝 + FILL/RX ring 所有权交接 |
| 队列粒度旁路 | 无此概念 | AF_XDP 可按队列旁路，与内核栈共存 |
| 内存约束 | 只受进程地址空间限制 | 追加：页对齐、`RLIMIT_MEMLOCK`、常驻不换出 |

---

→ 本篇：[03 UMEM 布局与 copy/zc 差异](03-af-xdp-umem-layout.md)
→ 前一篇：[01 AF_XDP 接口精读](01-af-xdp.md) · [02 AF_XDP 工程实践](02-af-xdp-lwn.md)
→ 相关：[chapter-04 page_pool](../../chapter-04-page-pool/) · [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) · [chapter-07 XDP redirect 与 DPDK](../../chapter-07-xdp-redirect-dpdk/)
