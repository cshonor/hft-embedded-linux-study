# 《Linux Device Drivers Development》（John Madieu）评测与路线定位

> **书名：** *Linux Device Drivers Development: Develop customized drivers for embedded Linux*  
> **作者：** John Madieu  
> **成书内核：** 约 Linux **4.1–4.13**（2017）  
> **模块：** [21 README](./README.md) · **全书大纲：** [refs/modern-driver-practice/OUTLINE.md](./refs/modern-driver-practice/OUTLINE.md)（22 章）  
> 四书分工：[FOUR-BOOKS-OVERLAP](../11-embedded-boot-build/FOUR-BOOKS-OVERLAP.md)（代号 **D**）  
> **板卡：** 书内示例偏 i.MX6；框架与写法**适配树莓派 ARM64**（5.x+ 需少量 API 适配）

---

## 一句话结论

**嵌入式驱动实操刚需、不可替代**（写模块 / DTS / I2C·SPI / DMA）。  
纯用户态、不碰硬件 → 价值很低。  
顺序：**Primer → 本书动手 → LDD3 按需补原理**；∥ LKD（调度/内存/实时）。LDD3 代码勿照搬。

---

## 核心定位

| | |
|--|--|
| 类型 | **嵌入式 Linux 实战驱动教科书** |
| 风格 | 代码驱动、框架全覆盖；少空洞理论 |
| 平台 | ARM（i.MX 示例）→ 可迁树莓派 |
| 不讲 | Bootloader / rootfs / Yocto 系统工程（那是 Primer / MELP） |

---

## 优势（树莓派 + 嵌入式 + HFT 底层）

### 1. 从零到总线：线性完整

1. 内核编译、交叉编译、模块 `Makefile`（环境搭建）  
2. 驱动高频工具：`container_of`、链表、等待队列、锁、定时器、中断下半部  
3. 字符驱动 → Platform → **设备树 DTS**  
4. **I2C / SPI** 外设实操（传感器刚需）  
5. Regmap、IIO、GPIO、中断控制器、RTC / PWM / 输入 / 网卡  
6. 内存管理、**DMA**（低延迟相关）

### 2. 完整可编译示例

各章配套完整驱动（非碎片）：Hello 模块、字符设备、DTS+platform、I2C EEPROM、SPI ADC、GPIO 扩展、RTC、PWM 等 — 交叉编译后可上板改。

### 3. 设备树讲得细（树莓派刚需）

DTS 语法、phandle、资源解析、OF 匹配；写/编 dtb、读寄存器/中断/GPIO — 老书 board-file 时代已废，本章价值高。深挖另见 [21 驱动/DT](../12-device-drivers-dt/)。

### 4. 内核工具讲透（稳定 / 低延迟基础）

mutex vs spinlock、tasklet/workqueue、hrtimer、DMA 缓存一致性、`devm` 自动释放 — 写稳驱动的核心，也是 HFT 系统底层优化的地基之一。

### 5. 嵌入式工业子系统面宽

IIO、Regmap、Regulator、GPIO 控制器、中断级联、网卡等 — 工业采集 / 板级外设覆盖好。

---

## 局限（客观）

| 点 | 说明 |
|----|------|
| **内核 4.x** | 树莓派 5.15/6.x 有 API 微调（宏/函数改名）；**架构与思想不变**，代码需少量适配 |
| **少 PCIe 深度** | 重心在 I2C/SPI/Platform；PCIe/NVMe 需另补 |
| **不讲调度/VM 原理** | 只写驱动；CFS、缺页、PREEMPT_RT → 搭配 [LKD](../07-linux-kernel/) |
| **少用户态性能路径** | 无 epoll/mmap 批量低延迟专项；仅基础 poll/ioctl → 用户态仍靠 TLPI / UNP / DPDK |

---

## vs Embedded Linux Primer（B）

| | Primer（Hallinan） | Madieu（本书） |
|--|-------------------|----------------|
| 擅长 | U-Boot、rootfs、Buildroot/Yocto、交叉环境、调试链 | 从零写驱动、DTS、I2C/SPI/GPIO、DMA、驱动框架 |
| 短板 | 几乎无完整驱动代码 | **不讲**系统构建 / Bootloader / rootfs |
| 用途 | LFS / 树莓派系统定制、整机流程 | 外接硬件驱动、内核硬件抽象层 |

→ **不重复**：一个搭系统，一个写驱动。

---

## 搭配读序（本仓库）

### 前置

- C：指针 / 内存  
- Linux 基础命令  
- 用户态：`read`/`write`/`ioctl`（[TLPI](../04-linux-userspace-api/) 已覆盖则可）

### 顺序

```
1. Embedded Linux Primer（+ MELP 实操）  → Boot→内核→rootfs、交叉环境
2. 本书 Madieu                            → 模块 / 字符 / DTS / I2C·SPI（树莓派动手）
3. LDD3（卡住锁/DMA/内存时回头）           → [LDD3-EVAL](./refs/LDD3-EVAL.md) · [OUTLINE](./refs/classic-driver-theory/OUTLINE.md)
4. ∥ LKD                                   → 进程、调度、内存、实时（HFT / PREEMPT_RT）
```

### 树莓派实操建议

1. I2C/SPI 章：外接 EEPROM / ADC，交叉编译上板  
2. DTS 章：改 dtb，自定义 GPIO / 外设资源  
3. DMA 章：结合高速传输理解缓存一致性（PCIe SSD 需另找资料加深）

---

## 什么时候必读 / 可跳

| 目标 | 本书 |
|------|------|
| 树莓派底层、自制外设驱动、ARM 嵌入式硬件 | **核心工具书** |
| 只学用户态 Linux、不碰硬件驱动 | 可跳 / 极低优先级 |
| HFT 纯用户态低延迟 | 非主线；DMA/锁章节可当补强 |

---

## 仓库内链接

- 模块总览：[README](./README.md)  
- **全书 22 章大纲：** [refs/modern-driver-practice/OUTLINE.md](./refs/modern-driver-practice/OUTLINE.md)  
- 与 LDD3 / Primer / MELP：[FOUR-BOOKS-OVERLAP](../11-embedded-boot-build/FOUR-BOOKS-OVERLAP.md)  
- 设备树：[README 设备树节](./README.md) · Madieu Ch6  
- 内核原理：[04 LKD](../07-linux-kernel/)
