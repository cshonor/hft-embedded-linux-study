# Linux Device Drivers Development · 全书大纲（Madieu）

> **John Madieu** · Packt · **22 章**  
> 内核：**4.1–4.13** · 示例：i.MX6 UDOO · 源码随书  
> 评测：[MADIEU-EVAL.md](../refs/MADIEU-EVAL.md) · 笔记树：本目录 `chapter-*/`

## 章目录一览

| Ch | 目录 |
|----|------|
| 1 | [chapter-01-introduction-to-kernel-development](./chapter-01-introduction-to-kernel-development/) |
| 2 | [chapter-02-device-driver-basis](./chapter-02-device-driver-basis/) |
| 3 | [chapter-03-kernel-facilities-helpers](./chapter-03-kernel-facilities-helpers/) |
| 4 | [chapter-04-character-device-drivers](./chapter-04-character-device-drivers/) |
| 5 | [chapter-05-platform-device-drivers](./chapter-05-platform-device-drivers/) |
| 6 | [chapter-06-device-tree](./chapter-06-device-tree/) |
| 7 | [chapter-07-i2c-client-drivers](./chapter-07-i2c-client-drivers/) |
| 8 | [chapter-08-spi-device-drivers](./chapter-08-spi-device-drivers/) |
| 9 | [chapter-09-regmap-api](./chapter-09-regmap-api/) |
| 10 | [chapter-10-iio-framework](./chapter-10-iio-framework/) |
| 11 | [chapter-11-kernel-memory-management](./chapter-11-kernel-memory-management/) |
| 12 | [chapter-12-dma](./chapter-12-dma/) |
| 13 | [chapter-13-linux-device-model](./chapter-13-linux-device-model/) |
| 14 | [chapter-14-pinctrl-gpio](./chapter-14-pinctrl-gpio/) |
| 15 | [chapter-15-gpio-controller-drivers](./chapter-15-gpio-controller-drivers/) |
| 16 | [chapter-16-advanced-irq-management](./chapter-16-advanced-irq-management/) |
| 17 | [chapter-17-input-devices-drivers](./chapter-17-input-devices-drivers/) |
| 18 | [chapter-18-rtc-drivers](./chapter-18-rtc-drivers/) |
| 19 | [chapter-19-pwm-drivers](./chapter-19-pwm-drivers/) |
| 20 | [chapter-20-regulator-framework](./chapter-20-regulator-framework/) |
| 21 | [chapter-21-framebuffer-drivers](./chapter-21-framebuffer-drivers/) |
| 22 | [chapter-22-nic-drivers](./chapter-22-nic-drivers/) |

## 阅读标签

| 标签 | 含义 |
|------|------|
| **精读** | 树莓派 / 嵌入式驱动主线 |
| **选读** | 子系统按需（RTC/PWM/FB/NIC…） |
| **速览** | 旧板文件注册等历史路径 |

---

## Preface 前言

适用内核 4.1–4.13、UDOO/i.MX6、配套源码、读者前置（C / 基础 Linux）。无细分小节。

---

## Ch1 Introduction to Kernel Development

| 节 | 重点 | 标签 |
|----|------|------|
| 1.1 Environment setup | 开发环境 | **精读** |
| 1.2 Kernel habits | 内核习惯 | 选读 |
| 1.3 Getting the sources | 取源码 | 精读 |
| 1.4 Kernel configuration | 配置 | **精读** |
| 1.5 Building your kernel | 编译 | **精读** |
| 1.6 Coding style | 编码规范 | 精读 |
| 1.7 Source organization | 源码目录 | 精读 |
| 1.8 Structure allocation/init | 结构体分配初始化 | 选读 |
| 1.9 Classes, objects, OOP | 内核 OOP 思想 | 选读 |

---

## Ch2 Device Driver Basis

| 节 | 重点 | 标签 |
|----|------|------|
| 2.1 User / kernel space | 隔离 | **精读** |
| 2.2 Modules | 模块概念 | **精读** |
| 2.3 Module dependencies | depmod | 精读 |
| 2.4 Loading / unloading · 2.4.1–2.4.2 | insmod；`/etc/modules-load.d` | **精读** |
| 2.5 Driver skeletons | 最简模板 | **精读** |
| 2.6 Entry / exit | init/exit | **精读** |
| 2.7 `__init` / `__exit` | 属性 | 精读 |
| 2.8 Module information | MODINFO | 选读 |
| 2.9 Licensing | GPL | 精读 |
| 2.10–2.12 Errors / printk / pr_* | 错误码、空指针、日志 | **精读** |
| 2.13 Module parameters | 传参 | 精读 |
| 2.14 Building · 2.14.1–2.14.2 | 树内 / 树外模块 | **精读** |

---

## Ch3 Kernel Facilities and Helper Functions

| 节 | 重点 | 标签 |
|----|------|------|
| 3.1 `container_of` | 核心宏 | **精读** |
| 3.2 Linked lists · 3.2.1–3.2.4 | 双向循环链表 CRUD/遍历 | **精读** |
| 3.3–3.4 Sleep / wait queue | 等待队列 | **精读** |
| 3.5 Delay & timers · 3.5.1–3.5.3 | jiffies/HZ、HRT、tickless | **精读** |
| 3.6 Delays and sleep | 原子 vs 可睡眠上下文 | **精读** |
| 3.7 Locking · 3.7.1–3.7.2 | mutex vs spinlock | **精读** |
| 3.8 Work deferring · 3.8.1–3.8.3 | softirq / tasklet / workqueue | **精读** |
| 3.9 Interrupts · 3.9.1–3.9.5 | 注册、锁规则、底半部、threaded IRQ、内核调用户态 | **精读** |

---

## Ch4 Character Device Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 4.1–4.2 Major/minor · 分配 | 设备号 | **精读** |
| 4.3 `struct file` / `inode` | 内核表示 | **精读** |
| 4.4 File operations | fops | **精读** |
| 4.5 `copy_to/from_user` | 用户↔内核拷贝 | **精读** |
| 4.6–4.10 open/release/write/read/llseek | 标准路径 | **精读** |
| 4.11 poll/select | 阻塞轮询 | **精读**（对照 TLPI/用户态） |
| 4.12 ioctl · 4.12.1 | 命令宏 | **精读** |

---

## Ch5 Platform Device Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 5.1 Platform 概念 | 片上无总线设备 | **精读** |
| 5.2 devices / data / resources | 结构 | **精读** |
| 5.3 旧板级 C 注册 · 5.3.1–5.3.2 | **已淘汰**，只为读老代码 | 速览 |
| 5.4 现代 DT 匹配 | OF | **精读** |
| 5.5 四种匹配 | name / ID / ACPI / OF | 精读 |

---

## Ch6 The Concept of a Device Tree

| 节 | 重点 | 标签 |
|----|------|------|
| 6.1 语法 · label/phandle | 基础 | **精读**（树莓派刚需） |
| 6.2 寻址 reg / #address-cells | | **精读** |
| 6.3 I2C/SPI/Platform 节点 | | **精读** |
| 6.4 资源解析 | 中断/时钟/GPIO/寄存器 | **精读** |
| 6.5 自定义属性 | 字符串/整数/布尔 | 精读 |
| 6.6 Platform + OF 匹配 | | **精读** |
| 6.7 兼容旧板级数据 | | 速览 |

→ 深化：[21 README · 设备树](../README.md) · 官方 usage-model

---

## Ch7 I2C Client Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 7.1 `i2c_driver` / `i2c_client` | 架构 | **精读** |
| 7.2 probe / remove | | **精读** |
| 7.3 通信 API · SMBus | | **精读** |
| 7.4 旧板文件注册 | | 速览 |
| 7.5 DTS 声明从设备 | | **精读** |

---

## Ch8 SPI Device Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 8.1 `spi_driver` | | **精读** |
| 8.2 读写传输 | | **精读** |
| 8.3 DTS 配置 | | **精读** |
| 8.4 用户态 SPI | | 选读 |

---

## Ch9 Regmap API

| 节 | 重点 | 标签 |
|----|------|------|
| 9.1 `regmap_config` | | 精读 |
| 9.2 SPI/I2C 初始化 | | 精读 |
| 9.3 读写 / 批量更新 | | 精读 |
| 9.4 缓存 | | 选读 |

---

## Ch10 IIO Framework

| 节 | 重点 | 标签 |
|----|------|------|
| 10.1 `iio_dev` | ADC/DAC 框架 | 选读（采集项目精读） |
| 10.2 通道 / 触发 / 缓冲 | | 选读 |
| 10.3 sysfs | | 选读 |
| 10.4 单次/连续采集示例 | | 选读 |

---

## Ch11 Kernel Memory Management

| 节 | 重点 | 标签 |
|----|------|------|
| 11.1 虚址 / 高低内存 | | 精读 |
| 11.2 VMA / MMU | | 精读（配 LKD/CSAPP） |
| 11.3 页分配器 · Slab · kmalloc/vmalloc | | **精读** |
| 11.4 `ioremap` | | **精读** |
| 11.5 `mmap` 到用户态 | | **精读**（HFT 相关） |
| 11.6 CPU/页缓存 | | 选读 |
| 11.7 `devres` | 托管资源 | **精读** |

---

## Ch12 DMA

| 节 | 重点 | 标签 |
|----|------|------|
| 12.1 缓存一致性 | | **精读**（低延迟） |
| 12.2 一致性 / 流式映射 | | **精读** |
| 12.3 scatter/gather | | 精读 |
| 12.4 DMA Engine | | 精读 |
| 12.5 DTS 绑定 DMA | | 精读 |

> 无 PCIe 专章；NVMe/PCIe 另查内核 PCI 文档。

---

## Ch13 The Linux Device Model

| 节 | 重点 | 标签 |
|----|------|------|
| 13.1 kobject / kset / sysfs | | 精读 |
| 13.2 bus / device / driver | | **精读** |
| 13.3 sysfs 属性 | | 精读 |

---

## Ch14 Pin Control and GPIO Subsystem

| 节 | 重点 | 标签 |
|----|------|------|
| 14.1 pinctrl | 引脚复用 | **精读**（树莓派） |
| 14.2 传统整数 GPIO | | 速览 |
| 14.3 描述符式 GPIO（推荐） | | **精读** |
| 14.4 GPIO 中断 · DTS | | **精读** |

---

## Ch15 GPIO Controller Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 15.1 `gpio_chip` | 控制器实现 | 选读 |
| 15.2 DTS / sysfs | | 选读 |

---

## Ch16 Advanced IRQ Management

| 节 | 重点 | 标签 |
|----|------|------|
| 16.1 IRQ 控制器 · `irq_domain` | | **精读** |
| 16.2 链式 / 嵌套中断 | | 精读 |
| 16.3 GPIO 作中断控制器案例 | | 精读 |

---

## Ch17 Input Devices Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 17.1 `input_dev` · 事件上报 | | 选读 |
| 17.2 轮询型输入 | | 选读 |
| 17.3 用户态读事件 | | 选读 |

---

## Ch18 RTC Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 18.1 RTC API · 闹钟 | | 选读 |
| 18.2 hwclock | | 选读 |

---

## Ch19 PWM Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 19.1 PWM 控制器 / 消费者 | | 选读（电机/调光精读） |
| 19.2 sysfs 控制 | | 选读 |

---

## Ch20 Regulator Framework

| 节 | 重点 | 标签 |
|----|------|------|
| 20.1 PMIC 驱动 | | 选读 |
| 20.2 获取 / 开关 / 调压 | | 选读 |

---

## Ch21 Framebuffer Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 21.1 `fb_ops` | | 选读 |
| 21.2 用户态访问显存 | | 选读 |

---

## Ch22 Network Interface Card Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 22.1 `sk_buff` | | 选读（接 [13 内核网络](../../14-kernel-networking/)） |
| 22.2 open/close / 收发包 | | 选读 |
| 22.3 ethtool | | 选读 |

---

## 学习递进（全书逻辑）

```
Ch2 模块 → Ch3 通用工具 → Ch4 字符设备
  → Ch5/6 Platform + DT → Ch7/8 I2C/SPI
  → Ch9/10 Regmap/IIO → Ch11/12 内存/DMA
  → Ch13 LDM → Ch14/16 GPIO/中断
  → Ch15/17–22 按外设选读
```

## 树莓派重点章

**2、3、4、5、6、7、8、14、16**（模块 / 工具 / 字符 / Platform+DT / I2C·SPI / GPIO·IRQ）

## 局限提醒

- **无 PCIe 专章** — NVMe/PCIe 另查内核 PCI 子系统文档  
- 5.x/6.x API 有微调 — 思想不变，代码需适配  
- 调度 / CFS / PREEMPT_RT → [LKD](../../05-linux-kernel/)

## 最短路径（动手）

```
Ch1–2 环境与模块 → Ch3 工具 → Ch4 字符设备
  → Ch5–6 Platform+DTS → Ch7 或 Ch8 外设一条线
  → Ch14 GPIO → 需要再开 Ch11–12 / Ch16
```
