## 5. BAR0 与 xHC 初始化

---

### 一、BAR0 — MMIO 基址

**BAR（Base Address Register）** — 告诉 OS：**控制该设备要映射到哪段物理地址**。

| 步骤 | 说明 |
|------|------|
| 读 PCI BAR0 | 得 **MMIO 物理基址**（可能需先写全 1 探测大小 — 规范流程） |
| **Memory Space** | xHCI 使用 **内存映射 IO** — 用 **指针读写寄存器** |
| 映射使用 | `volatile uint32_t* xhc_regs = (uint32_t*)bar0_phys;` |

```
CPU 读写 [bar0 + offset]  →  xHC 硬件寄存器
```

**与 PCI `in/out` 对比：** 配置空间用 **端口 IO**；xHC **运行时** 用 **MMIO**。

→ [Ch2 memmap](../chapter-02-edk2-memmap/notes/section-3-主存储器与内存映射.md) — 勿与 **普通 RAM** 混淆

---

### 二、xHC 初始化（概要）

| 阶段 | 典型操作 |
|------|----------|
| **复位 / HCHalted** | 确认控制器停止 |
| **分配数据结构** | **Device Context、Event Ring、Command Ring** 等（xHCI 规范） |
| **写 CAPLENGTH / RTSCR** | 运行寄存器集基址 |
| **RUN 位置位** | 启动控制器 |
| **端口枚举** | 连接 **USB 设备（鼠标）** |

**本章深度：** 实现 **足够驱动鼠标** 的最小子集 — 非完整 Linux xhci 驱动。

---

### 三、Intel Panther Point：EHCI → xHCI 路由

部分 **Intel 芯片组（Panther Point 等）** 存在 **USB2/3 端口路由** 问题：

| 问题 | 处理 |
|------|------|
| 物理端口默认连 **EHCI** | 需写 **PCI 配置 / 芯片组特定寄存器** 切到 **xHCI** |
| 书中步骤 | 针对 **Intel** 平台的 **port routing** 配置 |

**意义：** 真机上 **只初始化 xHC 仍无设备事件** — 常因路由未切换。

---

### 四、设备选择策略

书中 **优先 Intel xHC** — 在 QEMU 与常见开发机上匹配率高；其他 Vendor 需扩展 **VID/DID 表**。

---

← [4. PCI](./section-4-PCI配置空间与枚举.md) · 下一节 [6. 轮询](./section-6-轮询输入与遗留问题.md)
