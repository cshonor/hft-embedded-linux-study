## 3. 基于 DPDK 的用户态 vhost 设计

> DPDK 以 **vhost-user** 实现用户态后端 — 控制面 Socket + 数据面 **共享内存**

---

### 一、消息机制（Unix Domain Socket）

vhost-user 进程创建 **Unix socket server**，Qemu（或 VMM）为 **client**。

| 典型消息 | 作用 |
|----------|------|
| `VHOST_SET_FEATURES` | **特性协商** — 与 [Ch11](../chapter-11-virtio-paravirtualization/notes/section-2-Virtio规范与使用场景.md) 前端一致 |
| `VHOST_SET_OWNER` | 所有权 |
| `VHOST_SET_MEM_TABLE` | **Guest 内存布局** — GPA/文件偏移 |
| `VHOST_SET_VRING_*` | 队列地址、kick/eventfd |
| eventfd | **中断/ kick 通知** — 可配合 poll 减唤醒 |

**分工：** Socket = **慢路径配置**；数据面 **不经过** Socket。

---

### 二、地址转换与虚拟机内存映射

**问题：** vhost 进程须 **直接读写** Guest 的：

- Descriptor / Available / Used 环  
- **报文缓冲区**（Guest 物理地址）

**做法：**

1. Qemu 用 **`mem-path`** 等在宿主机 **大页文件系统** 分配 Guest RAM  
2. Qemu 经 `VHOST_SET_MEM_TABLE` 告知 **区域列表**（guest_phys_addr, size, userspace_addr, mmap_offset）  
3. vhost（DPDK）**mmap** 同一 backing file → **零拷贝** 访问 Guest 内存  

| 关键 | 说明 |
|------|------|
| **大页** | [Ch2](../chapter-02-cache-and-memory/notes/section-5-大页Hugepages.md) · [Ch10 EPT/IOTLB](../chapter-10-x86-io-virtualization/notes/section-4-透传下收发包流程.md) |
| **零拷贝** | vhost 读写的即是 Guest mbuf 缓冲 — 无 Qemu 中转 |
| **安全** | 仅 vhost 与 Qemu 约定区域 — 需正确 **mem_table** |

---

### 三、数据通路（概念）

```
Guest net_virtio TX → Available Ring
    → vhost-user（DPDK）poll Used/Available
    → 宿主机 DPDK app：转发 / OVS / NIC TX
    （反向 RX 对称）
```

与 [Ch11 Virtqueue §3](../chapter-11-virtio-paravirtualization/notes/section-3-虚拟队列机制.md) **同一套环**，后端在 **Host 用户态** 消费。

---

← [2. vhost 演进](./section-2-vhost演进与原理.md) · 下一节 [4. 编程封装](./section-4-vhost编程与封装.md)
