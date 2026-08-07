# 04 — Documentation/networking/af_xdp.rst

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/networking/af_xdp.rst`

## 文档概述

AF_XDP socket 官方文档，描述用户态零拷贝收发包的完整接口。

## 核心内容

### AF_XDP socket 创建流程

```
1. socket(AF_XDP, SOCK_RAW, 0)
2. 分配 UMEM（mmap 或 malloc + posix_memalign）
3. setsockopt(XDP_UMEM_REG) 注册 UMEM
4. 创建四个 ring（FILL/RX/TX/COMPLETION）
5. setsockopt(XDP_UMEM_FILL_RING / XDP_UMEM_RX_RING / ...)
6. bind(sockfd, sockaddr_xdp{ifindex, queue_id, flags})
7. 预填充 FILL ring（提供空闲 buffer）
8. 轮询 RX ring 收包
```

### 两种模式

| 模式 | flag | 说明 |
|------|------|------|
| Copy | 无 | 内核拷贝包到 UMEM，兼容所有驱动 |
| Zero-copy | XDP_ZEROCOPY | page_pool 直接映射，需驱动支持 |

### 驱动支持情况

| 驱动 | copy | zero-copy |
|------|------|-----------|
| mlx5 | ✅ | ✅ |
| ice | ✅ | ✅ |
| i40e | ✅ | ✅ |
| stmmac | ✅ | ✅（5.x+） |
| virtio-net | ✅ | ✅ |

### 统计信息

```bash
# 查看 AF_XDP socket 统计
xdp-stat queue 0
```
| 统计项 | 含义 |
|--------|------|
| rx_dropped | FILL ring 空导致丢包 |
| rx_invalid_descs | 无效描述符 |
| tx_invalid_descs | 发送无效描述符 |

## HFT 要点

- 零拷贝模式是 HFT 用 AF_XDP 的唯一理由（copy 模式不如直接 recvmsg）
- 绑定到特定 RX 队列：`ethtool -L eth0 combined N` 设置队列数
- FILL ring 预填充数量需 > ring 大小 / 2，避免空 buffer 丢包
