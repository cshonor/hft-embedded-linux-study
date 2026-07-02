## 4. PCI 配置空间与设备枚举

> **xHC 是 PCI 设备** — 必须先 **在 PCI 总线上找到它**。

---

### 一、PCI 配置空间

每个 PCI 功能有 **256 字节（或扩展）配置空间**，含：

| 字段（偏移示意） | 用途 |
|------------------|------|
| **Vendor ID / Device ID** | 厂商与型号 — 如 **Intel** |
| **Class Code** | 设备 **类别** — 筛选 USB 主机控制器 |
| **BAR0–BAR5** | **基地址寄存器** — MMIO 或 IO 端口基址 |
| **Command / Status** | 使能总线 master、IO/MEM 空间等 |

**Class Code 示例（USB xHCI）：**

- Base Class **0x0C**（Serial Bus）
- Sub-class **0x03**（USB）
- Interface **0x30**（xHCI）— 以规范/书为准

---

### 二、CONFIG_ADDRESS / CONFIG_DATA 访问

传统 x86 **配置机制**（Type 1）：

```
1. 向 CONFIG_ADDRESS (0xCF8) out 写：
   bit31=1 | bus | device | function | register_offset
2. 从 CONFIG_DATA (0xCFC) in/out 读写字节/字/双字
```

| 指令 | 说明 |
|------|------|
| **`out`** | CPU → I/O 端口写 |
| **`in`** | I/O 端口 → CPU 读 |

**书中实现：** 封装 `WritePciConfigAddress` / `ReadPciConfig*` — 内核 **汇编或内联** 访问。

→ [CSAPP Ch3 控制](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/) · [02 PCIe 章节](../../../02-Computer-Architecture-6th/)

---

### 三、枚举算法（简化）

```
for bus in 0..255:
  for device in 0..31:
    for function in 0..7:
      read Vendor ID
      if Vendor == 0xFFFF → empty slot
      read Class Code
      if matches xHCI → 候选 xHC
```

| 策略 | 本书 |
|------|------|
| 多个 xHC | **优先 Intel** 设备（书中实践） |
| QEMU OVMF | 通常能枚举到 **虚拟 xHCI** |

找到设备后 → 读 **BAR0**（[5. 初始化](./section-5-BAR0与xHC初始化.md)）。

---

← [3. USB / xHCI](./section-3-USB分层与xHCI.md) · 下一节 [5. BAR0](./section-5-BAR0与xHC初始化.md)
