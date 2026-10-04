# XDP / tc-BPF 与 DPDK 对照

> **02-Advanced-Book** · 《Linux 高性能网络详解》配套 · **选读**

> **本篇分工：** [note-openonload-rdma对比](./note-openonload-rdma对比.md) 讲**绕开内核**的路线；
> 本篇讲**留在内核里**能快到什么程度（XDP）、以及半绕开的 AF_XDP 怎么当"低风险第一步"。
> eBPF/XDP 的工具链与观测深入在 [06.7-BPF](../../../06.7-bpf-observability/note-XDP与tc-BPF.md)，本篇只落到**与 DPDK 的取舍**。

---

## 一、与 DPDK 的分工

| | DPDK（01-Intro） | XDP / tc-BPF | **AF_XDP** |
|---|------------------|--------------|-----------|
| 位置 | 用户态完全旁路 | 内核最早 hook 点 | 内核 hook + 用户态收包 |
| 网卡归属 | **归 DPDK**，内核看不到 | 归内核 | **归内核**，只把指定流重定向出来 |
| 生效粒度 | 整张网卡 | 每张网卡一个程序 | **可只旁路一条流**，其余照常 |
| 典型用途 | UDP 组播行情、极致 poll | 早期过滤、统计、DDoS 防护 | 渐进式旁路，可回退 |
| 延迟量级 | ~0.3–1μs | ~1–5μs（内核内处理） | zc ~0.5–2μs；**copy 模式无意义** |
| 开发成本 | 高（无 socket 语义） | 中（仍在 eBPF 生态） | 中（ring + UMEM 要自己管） |

**AF_XDP 是两者之间的桥**——这也是它最实用的地方：

- 想提速但**不能动基础设施**（管理流量、SSH、监控都还在）→ AF_XDP
- 已有系统先旁路一条行情流试水，效果满意再考虑整卡切 DPDK → AF_XDP 是低风险第一步
- 全新专用接入卡、要最后那点确定性 → DPDK

---

## 二、XDP 程序骨架：网卡驱动里的第一个 hook

XDP 程序在**驱动收包路径上、`sk_buff` 分配之前**执行——这是它比 tc-BPF 快的根本原因
（tc 已经有 skb 了，XDP 面对的是裸 `xdp_buff`）：

```c
/* xdp_prog.c —— 用 clang -target bpf 编译 */
SEC("xdp")
int mcast_filter(struct xdp_md *ctx)
{
    void *data     = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    /* 边界检查是强制的：verifier 要求每次访存前先证明不越界 */
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    /* ... 解析 IP/UDP，判断是不是行情组播流 ... */

    if (is_market_data)
        return XDP_REDIRECT;   /* 配合 cpumap/devmap 或 AF_XDP socket 重定向 */
    if (is_garbage)
        return XDP_DROP;       /* 最早点丢弃：DDoS/垃圾流量零 skb 开销 */
    return XDP_PASS;           /* 其余照常进内核栈 */
}
```

五个返回值就是全部"动作语义"：

| action | 含义 | HFT 用法 |
|---|---|---|
| `XDP_PASS` | 进内核栈 | 默认：不认识的流量不碰 |
| `XDP_DROP` | 就地丢弃 | DDoS/非目标组播组最早点过滤 |
| `XDP_TX` | 从同网卡发回去 | 心跳/应答的极简反射 |
| `XDP_REDIRECT` | 转发到别的网卡/CPU/AF_XDP socket | **把行情流送进 AF_XDP** |
| `XDP_ABORTED` | 出错丢弃（调试占位） | 不应出现在生产路径 |

加载与查看：

```bash
# native 模式（驱动内执行，最快；需要驱动支持）
sudo ip link set dev eth0 xdp obj xdp_prog.o sec xdp
# 查看已加载程序
sudo bpftool prog show
sudo bpftool net
```

---

## 三、AF_XDP 骨架：ring + UMEM 全自己管

AF_XDP = XDP_REDIRECT 把指定流导进一个特殊 socket，
应用通过 **4 个无锁环形队列**直接收发报文——语义上就是"内核版 DPDK 描述符环"：

```c
struct xsk_umem_config umem_cfg = {
    .fill_size = 2048, .comp_size = 2048, .frame_size = 4096,
    .frame_headroom = 0,
};
/* UMEM：一大块注册内存，切成等长 frame，内核和应用共享 */
xsk_umem__create(&umem, area, area_size, &fill, &comp, &umem_cfg);

struct xsk_socket_config xsk_cfg = {
    .rx_size = 2048, .tx_size = 2048,
    .bind_flags = XDP_ZEROCOPY,        /* ← 关键，见下面陷阱 */
    .xdp_flags = XDP_FLAGS_DRV_MODE,   /* native 模式 */
};
xsk_socket__create(&xsk, "eth0", queue_id, umem, &rx, &tx, &xsk_cfg);
```

收包循环（和 DPDK 的 burst 神似，连 prefetch 思路都一样）：

```c
uint32_t idx;
uint32_t rcvd = xsk_ring_cons__peek(&rx, BURST, &idx);   /* 看有多少 */
for (uint32_t i = 0; i < rcvd; i++) {
    const struct xdp_desc *desc = xsk_ring_cons__rx_desc(&rx, idx + i);
    uint8_t *pkt = xsk_umem__get_data(umem_area, desc->addr);
    parse(pkt, desc->len);
    xsk_umem__free_addr(desc->addr);     /* 归还 frame 到 fill 环 */
}
xsk_ring_cons__release(&rx, rcvd);
```

⚠ **AF_XDP 的 copy 模式是个陷阱**：`bind_flags` 不带 `XDP_ZEROCOPY`
（或驱动不支持回退到 copy）时，仍然会分配 `sk_buff` 并做一次 memcpy，
只是省了协议栈——**延迟和内核栈精调拉不开差距，白折腾**。
只有 zero-copy（驱动支持 + 绑定正确队列）才有意义。
→ [12.5/chapter-06/notes/03-af-xdp-umem-layout](../../../12.5-modern-networking/chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md)

**zero-copy 驱动支持**（内核主线）：ixgbe、i40e、ice、mlx5、veth（部分）。
Intel 中高端卡全覆盖，Mellanox/NVIDIA 有 mlx5——选型时先确认网卡驱动在列表里。

---

## 四、组合用法（生产环境常见）

```
行情口（独占网卡）  → DPDK 收包，解析，喂策略
管理口（另一张卡）  → 内核栈 + XDP 做过滤/DDoS 防护/统计
```

**这两者不冲突，是分工。** XDP 在管理面继续做它擅长的事（内核内早期处置），
DPDK 在数据面拿确定性。别把 XDP 装到 DPDK 接管的网卡上——那张卡内核已经看不到了。

渐进路线也成立：

```
内核栈（现状）
  → XDP 过滤垃圾流量（管理面收益，零风险）
  → AF_XDP 旁路一条行情流试水（可回退：删 socket 就回内核栈）
  → 效果确认后，专用卡整卡切 DPDK（不可逆，要重写解析层）
```

每一步都可独立验证、独立回退——这是它比"一步到位上 DPDK"更适合老系统的原因。

---

## 五、相关章节

- 深入 XDP 实现与工具：[06.7-BPF note-XDP与tc-BPF](../../../06.7-bpf-observability/note-XDP与tc-BPF.md)
- AF_XDP UMEM 布局：[12.5/chapter-06/notes/03-af-xdp-umem-layout](../../../12.5-modern-networking/chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md)
- 旁路后的完整链路：[01-Intro chapter-04 零拷贝与用户态旁路](../../01-Intro-Book/notes/chapter-04-零拷贝与用户态旁路.md)
- 方案总表：[note-openonload-rdma对比](./note-openonload-rdma对比.md)
- 上一梯度：[01-Intro-Book](../../01-Intro-Book/)
- [13 总目录](../../README.md)
