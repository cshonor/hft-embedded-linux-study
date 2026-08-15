# 《Linux 设备驱动程序》第三版（LDD3）评测与路线定位

> **英文：** *Linux Device Drivers, Third Edition*  
> **作者：** Jonathan Corbet · Alessandro Rubini · Greg Kroah-Hartman  
> **内核：** 约 **2.6.10**（2005）  
> **大纲：** [refs/classic-driver-theory/OUTLINE.md](./classic-driver-theory/OUTLINE.md)（18 章）  
> 四书代号 **C**：[FOUR-BOOKS-OVERLAP](../08-embedded-boot-build/FOUR-BOOKS-OVERLAP.md)

---

## 一句话结论

**经典驱动思想圣经** — 锁 / 并发 / 中断 / DMA / 设备模型讲得最透。  
树莓派实操 **不要当主教材**（无 DTS、无 I2C/SPI、API 过时）。  
用法：Madieu 动手卡住时，**回头精读 LDD3 对应原理章**。

---

## 定位

| | |
|--|--|
| 类型 | 内核驱动入门圣经；原理 + 通用框架 |
| 强项 | scull 无硬件可练；PCI/USB/块/网络详尽；Greg 为 USB 维护者 |
| 弱项 | 2.6 极老；**无设备树**；无 ARM 外设总线实战 |

---

## 优势

1. **底层原理最深**（三本驱动相关书里）：锁、内存、中断、DMA、LDM  
2. PC 标准总线完整：PCI / USB / 块 / 网络  
3. **scull** 纯内存字符设备 — PC 上零硬件可调试  
4. 并发、竞态、内存屏障、缓存一致性 — **HFT / 高速硬件理论基石**

---

## 致命局限（树莓派）

1. **无 DTS** — 树莓派硬件配置全靠设备树  
2. **无 ARM / I2C / SPI** 章节  
3. 无 `devm_*`、`regmap` 等现代 API  
4. 无 U-Boot / rootfs / Yocto / 交叉编译（那是 Primer / MELP）

---

## 与另外两本分工

| 书 | 角色 |
|----|------|
| [Embedded Linux Primer](../08-embedded-boot-build/primer-system-overview/) | 系统全景：Boot / rootfs / 交叉环境 |
| [Madieu](./modern-driver-practice/) | ARM · DTS · I2C/SPI/GPIO **实操**（树莓派主书） |
| **LDD3（本书）** | 原理补课：锁 / DMA / 内存 / 并发 / PCI·USB |

---

## 推荐读序（本仓库）

```
1. Primer（+ MELP）     → 系统怎么跑起来
2. Madieu               → 树莓派写模块 / DTS / I2C·SPI
3. LDD3（按需回头）     → 锁、DMA、内存、并发、PCI/USB 不懂时精读对应章
∥ LKD                   → 调度 / VM / PREEMPT_RT（HFT）
```

**不要**把 LDD3 样例当 5.x 模板硬抄。

---

## HFT 相关章（理论）

| 主题 | LDD3 章 |
|------|---------|
| 并发 / 锁 / RCU | Ch5 |
| poll / 阻塞 I/O | Ch6 |
| 定时 / 下半部 | Ch7 |
| 内存分配 | Ch8 |
| MMIO / barrier | Ch9 |
| 中断 | Ch10 |
| mmap / DMA | Ch15 |
| 网卡 / sk_buff | Ch17 |
