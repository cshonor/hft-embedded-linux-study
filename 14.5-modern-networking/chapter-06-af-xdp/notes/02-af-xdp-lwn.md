# 06 — AF_XDP：零拷贝到用户态

> **对应 Rosen:** 无
> **内核版本:** 4.18+（初始）；zero-copy 模式需驱动支持

## AF_XDP 是什么

AF_XDP（Address Family XDP）是一种 socket 类型，允许用户态程序直接从 XDP hook 接收包：
- XDP 程序将包 redirect 到 AF_XDP socket
- 用户态程序从共享内存环形缓冲区读取包
- 零拷贝模式下，page_pool 的 page 直接映射到用户态

## 架构

```
NIC DMA → page_pool → XDP hook → XDP_REDIRECT → AF_XDP socket
                                                    ↓
                                           UMEM（用户态共享内存）
                                           ↙              ↘
                                     FILL ring          RX ring
                                   (空闲 buffer)      (收到的包)

                                     TX ring          COMPLETION ring
                                   (待发送包)        (发送完成通知)
```

## 四个环形缓冲区

| Ring | 方向 | 作用 |
|------|------|------|
| FILL | 用户→内核 | 用户态提供空闲 buffer 给内核填充 |
| RX | 内核→用户 | 内核将收到的包放入 buffer，通知用户态 |
| TX | 用户→内核 | 用户态将待发送包放入 buffer |
| COMPLETION | 内核→用户 | 内核通知发送完成，buffer 可回收 |

## 两种模式

| 模式 | 机制 | 延迟 | 驱动要求 |
|------|------|------|---------|
| Copy mode | 内核拷贝包到 UMEM | 较高 | 所有 XDP 驱动 |
| Zero-copy mode | page_pool 直接映射 | 最低 | 驱动支持 zero-copy |

## 代码框架

```c
// 创建 AF_XDP socket
int sockfd = socket(AF_XDP, SOCK_RAW, 0);

// 注册 UMEM（共享内存区域）
struct xdp_umem_reg umem = {
    .addr = (uintptr_t)buffer,
    .len = BUFFER_SIZE,
    .chunk_size = 4096,
};
setsockopt(sockfd, SOL_XDP, XDP_UMEM_REG, &umem, sizeof(umem));

// 绑定到网卡+队列
struct sockaddr_xdp sxdp = {
    .sxdp_family = AF_XDP,
    .sxdp_ifindex = ifindex,
    .sxdp_queue_id = queue_id,
    .sxdp_flags = XDP_ZEROCOPY,  // 零拷贝模式
};
bind(sockfd, (struct sockaddr*)&sxdp, sizeof(sxdp));

// 用户态轮询收包
struct xdp_desc desc;
while (1) {
    while (xsk_ring_cons__has_data(&rx_ring)) {
        // 直接访问 UMEM 中的包数据，零拷贝
        void *pkt = xsk_umem__get_data(buffer, desc.addr);
        // 处理行情...
    }
}
```

## HFT 关联

| 维度 | AF_XDP 优势 |
|------|------------|
| 零拷贝 | 包数据不经过内核协议栈，直接到用户态 |
| 低延迟 | 接近 DPDK，但不需要独占网卡 |
| 内核共存 | 与其他内核网络功能（路由/TCP）共存 |
| 灵活切换 | 行情流走 AF_XDP，管理流走普通 socket |

## AF_XDP vs DPDK

| 维度 | AF_XDP | DPDK |
|------|--------|------|
| 零拷贝 | 是（page_pool 映射） | 是（大页 + VFIO） |
| 网卡独占 | 否（与其他队列共享） | 是 |
| CPU 占用 | 用户态轮询 | 用户态轮询 |
| 部署 | 加载 BPF + bind socket | 绑定 VFIO + 配 hugepage |
| 延迟 | 略高于 DPDK | 最低 |
| 适合 HFT | 中低频策略 | 超低延迟 co-location |
