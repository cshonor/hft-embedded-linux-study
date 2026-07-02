## 2. vhost 的演进与原理

---

### 一、传统 Qemu + virtio-net（最慢）

```
Guest virtio 前端
    → VM-Exit / KVM 消息
    → 用户态 Qemu
    → 读写 Tap 设备
    → 宿主机内核栈 …
```

| 瓶颈 | 说明 |
|------|------|
| **上下文切换** | Guest ↔ KVM ↔ **Qemu 用户态** |
| **多次拷贝** | virtqueue 缓冲 ↔ Qemu ↔ Tap |
| **CPU 占用** | 每包都陷出到 Qemu |

→ 为何需要 **后端下沉** — 把 virtqueue 处理移出 Qemu

---

### 二、Linux 内核态 vhost-net

**`vhost-net` 模块：** virtio-net **报文收发卸载到内核**

```
Guest virtqueue
    ↔ vhost-net（内核线程）
    ↔ Tap / 物理口
```

| 改进 | 说明 |
|------|------|
| **绕过 Qemu 数据面** | 控制仍由 Qemu，**数据** 在内核 vhost 线程 |
| 路径缩短 | Tap ↔ vhost-net **直接流转** |
| 局限 | 仍在 **内核态** — 与 DPDK 用户态哲学不一致 |

→ 内核 vhost 对照 [13-LKN](../../../11-Linux-Kernel-Networking/)

---

### 三、用户态 vhost-user

**目标：** 后端逻辑 **完全用户态** + 与 **DPDK 同址** 处理

| 特点 | 说明 |
|------|------|
| **Qemu 仅控制** | 特性、内存表、kick — 经 **Unix socket** |
| **DPDK 进程** | 轮询 virtqueue、转发/交换 — **零 syscall 数据面** |
| 生态 | OVS-DPDK、VPP、**vhost-switch** |

→ 下一节 DPDK 实现细节（§3）

---

### 四、演进对比

| 阶段 | 数据面位置 | 相对性能 |
|------|----------|----------|
| Qemu + Tap | 用户态 Qemu | 最低 |
| vhost-net | 内核 vhost | 中 |
| **vhost-user + DPDK** | **用户态 DPDK** | **高**（仍 < SR-IOV 透传） |

→ [Ch10 透传](../chapter-10-x86-io-virtualization/) · [Ch11 前端](../chapter-11-virtio-paravirtualization/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. DPDK vhost-user](./section-3-DPDK用户态vhost设计.md)
