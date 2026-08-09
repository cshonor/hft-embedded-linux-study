# Linux Device Drivers, 3rd ed（LDD3）· 全书大纲

> **Corbet / Rubini / Kroah-Hartman** · O'Reilly · **18 章**  
> 内核：**2.6.10** · **无设备树** · 评测：[LDD3-EVAL.md](../refs/LDD3-EVAL.md)

## 章目录一览

| Ch | 目录 |
|----|------|
| 1 | [chapter-01-introduction-to-device-drivers](./chapter-01-introduction-to-device-drivers/) |
| 2 | [chapter-02-building-and-running-modules](./chapter-02-building-and-running-modules/) |
| 3 | [chapter-03-character-device-drivers](./chapter-03-character-device-drivers/) |
| 4 | [chapter-04-debugging-techniques](./chapter-04-debugging-techniques/) |
| 5 | [chapter-05-concurrency-and-race-conditions](./chapter-05-concurrency-and-race-conditions/) |
| 6 | [chapter-06-advanced-char-driver-operations](./chapter-06-advanced-char-driver-operations/) |
| 7 | [chapter-07-time-delays-deferred-work](./chapter-07-time-delays-deferred-work/) |
| 8 | [chapter-08-allocating-memory](./chapter-08-allocating-memory/) |
| 9 | [chapter-09-communicating-with-hardware](./chapter-09-communicating-with-hardware/) |
| 10 | [chapter-10-interrupt-handling](./chapter-10-interrupt-handling/) |
| 11 | [chapter-11-kernel-data-types](./chapter-11-kernel-data-types/) |
| 12 | [chapter-12-pci-drivers](./chapter-12-pci-drivers/) |
| 13 | [chapter-13-usb-drivers](./chapter-13-usb-drivers/) |
| 14 | [chapter-14-linux-device-model](./chapter-14-linux-device-model/) |
| 15 | [chapter-15-memory-mapping-and-dma](./chapter-15-memory-mapping-and-dma/) |
| 16 | [chapter-16-block-drivers](./chapter-16-block-drivers/) |
| 17 | [chapter-17-network-drivers](./chapter-17-network-drivers/) |
| 18 | [chapter-18-tty-drivers](./chapter-18-tty-drivers/) |

## 阅读标签

| 标签 | 含义 |
|------|------|
| **精读** | 原理基石（并发/中断/DMA/字符驱动/LDM） |
| **选读** | PCI/USB/块/网络/TTY — 按总线需要 |
| **对照** | 与 Madieu 同主题对照；实现以 Madieu/5.x 为准 |

---

## Preface

全书定位、读者、内核版本、示例获取、前置（C + Unix syscall）。

---

## Ch1 An Introduction to Device Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 1.1 驱动程序的角色 | 机制与策略分离；设备即文件 | **精读** |
| 1.2 内核功能划分 · 1.2.1 模块 | 五大子系统；可加载模块 | 精读 |
| 1.3 设备与模块分类 | 字符 / 块 / 网络 | **精读** |
| 1.4 安全 | 权限、溢出、输入校验 | 精读 |
| 1.5 版本编号 | 内核版本规则 | 选读 |
| 1.6 GPL | 传染与模块许可 | 精读 |
| 1.7 社区 | 邮件列表、提交 | 速览 |
| 1.8 本书概要 | 18 章路线 | 速览 |

---

## Ch2 Building and Running Modules

| 节 | 重点 | 标签 |
|----|------|------|
| 2.1 测试系统 | 源码树、风险 | 选读 |
| 2.2 Hello World | `module_init/exit` | **精读** |
| 2.3 与应用差异 · 2.3.1–2.3.4 | 用户/内核、并发、`current`、小栈/无 libc/禁浮点 | **精读** |
| 2.4 编译加载 · 2.4.1–2.4.4 | Makefile/`obj-m`、insmod 族、vermagic、可移植 | **精读**（对照 Madieu Ch2） |
| 2.5 内核符号表 | `EXPORT_SYMBOL` | 精读 |
| 2.6 元数据宏 | LICENSE/AUTHOR… | 选读 |
| 2.7 init/cleanup · 2.7.1–2.7.3 | `__init/__exit`、goto 失败路径、加载竞态 | **精读** |
| 2.8 `module_param` | 传参 | 精读 |
| 2.9 用户态驱动 | libusb 等 | 选读 |

---

## Ch3 Character Device Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 3.1 scull 设计 | 无硬件可测 | **精读** |
| 3.2 `dev_t` 主/次设备号 | MKDEV；静/动态分配 | **精读** |
| 3.3 inode / file / fops | `private_data` | **精读** |
| 3.4 `cdev` 注册 | init/add/del | **精读**（对照 Madieu Ch4） |
| 3.5 open/release | | **精读** |
| 3.6 read/write | `copy_*_user` | **精读** |
| 3.7 scull 量子集 | 分片内存模型 | 选读 |
| 3.8 设备节点脚本 | `/proc/devices` + mknod | 选读 |

---

## Ch4 Debugging Techniques

| 节 | 重点 | 标签 |
|----|------|------|
| 4.1 CONFIG_DEBUG* | 毒化、栈、kallsyms | 选读 |
| 4.2 printk · 4.2.1–4.2.3 | 级别、过滤、ratelimit | **精读** |
| 4.3 /proc · seq_file | | 精读 |
| 4.4 strace | 定位 ioctl/读写 | 选读 |
| 4.5 Oops | 调用栈定位 | **精读** |
| 4.6 SysRq / gdb / KGDB / UML | | 选读 |

---

## Ch5 Concurrency and Race Conditions

| 节 | 重点 | 标签 |
|----|------|------|
| 5.1 scull 并发漏洞 | | **精读** |
| 5.2 四大来源 | 多进程/SMP/抢占/中断 | **精读** |
| 5.3 semaphore / mutex | 可睡眠 | **精读** |
| 5.4 completion | | 精读 |
| 5.5 spinlock（irq 系列） | 禁睡眠；中断上下文 | **精读**（HFT） |
| 5.6 rwsem / rwlock | | 选读 |
| 5.7 无锁 · 5.7.1–5.7.4 | atomic、位操作、seqlock、**RCU** | **精读** |

---

## Ch6 Advanced Char Driver Operations

| 节 | 重点 | 标签 |
|----|------|------|
| 6.1 ioctl | `_IO*` 宏、参数校验 | **精读** |
| 6.2 阻塞 / `O_NONBLOCK` | wait queue | **精读** |
| 6.3 poll/select/epoll | `poll_wait`、掩码 | **精读**（对照用户态 TLPI） |
| 6.4 fasync / SIGIO | | 选读 |
| 6.5 llseek | | 选读 |
| 6.6 `capable()` | | 选读 |

---

## Ch7 Time, Delays, and Deferred Work

| 节 | 重点 | 标签 |
|----|------|------|
| 7.1 jiffies / HZ | | **精读** |
| 7.2 mdelay / msleep | 忙等 vs 休眠 | **精读** |
| 7.3 `timer_list` | | 精读（现代多 HRT，对照 Madieu Ch3） |
| 7.4 下半部 · tasklet / workqueue | | **精读** |

---

## Ch8 Allocating Memory

| 节 | 重点 | 标签 |
|----|------|------|
| 8.1 kmalloc · GFP_* | KERNEL vs ATOMIC | **精读** |
| 8.2 slab | | 精读 |
| 8.3 get_free_pages · 高低端 | | 精读 |
| 8.4 vmalloc | | 精读 |
| 8.5 per-CPU | | 选读 |

---

## Ch9 Communicating with Hardware

| 节 | 重点 | 标签 |
|----|------|------|
| 9.1 I/O 端口 vs MMIO | | **精读** |
| 9.2 inb/outb | | 选读（x86） |
| 9.3 ioremap | | **精读** |
| 9.4 barrier | 防乱序 | **精读**（HFT） |

---

## Ch10 Interrupt Handling

| 节 | 重点 | 标签 |
|----|------|------|
| 10.1 request_irq / free_irq · SHARED | | **精读** |
| 10.2 顶半部 / 底半部 | | **精读** |
| 10.3 tasklet / workqueue 下半部 | | **精读** |
| 10.4 共享中断规范 | | 精读 |
| 10.5 中断驱动阻塞 I/O 示例 | | 选读 |

---

## Ch11 Kernel Data Types

| 节 | 重点 | 标签 |
|----|------|------|
| 11.1 u8/u32… | 可移植类型 | 精读 |
| 11.2 `list_head` · `container_of` | | **精读** |
| 11.3 可移植避坑 | | 选读 |

---

## Ch12 PCI Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 12.1 PCI 架构 · 配置空间 | | 选读（Pi PCIe/NVMe 另补现代文档） |
| 12.2 `pci_register_driver` | | 选读 |
| 12.3 资源读取 | | 选读 |
| 12.4 热插拔 · PCIe 概念 | | 选读 |

---

## Ch13 USB Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 13.1 拓扑 · 端点 · 描述符 | | 选读 |
| 13.2 URB | | 选读 |
| 13.3 驱动流程 | | 选读 |
| 13.4 批量/中断/控制传输 | | 选读 |

---

## Ch14 The Linux Device Model

| 节 | 重点 | 标签 |
|----|------|------|
| 14.1 kobject / kset / sysfs | | **精读**（对照 Madieu Ch13） |
| 14.2 bus / device / driver | | **精读** |
| 14.3 class · uevent | | 精读 |
| 14.4 固件加载 | | 选读 |

---

## Ch15 Memory Mapping and DMA

| 节 | 重点 | 标签 |
|----|------|------|
| 15.1 mmap · `remap_pfn_range` | | **精读**（HFT） |
| 15.2 direct I/O | | 选读 |
| 15.3 DMA · 一致性/流式/sg | 缓存一致性 | **精读**（对照 Madieu Ch12） |

---

## Ch16 Block Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 16.1 gendisk 注册 | | 选读 |
| 16.2 request_queue | | 选读 |
| 16.3 IO 调度 · bio | | 选读 |

---

## Ch17 Network Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 17.1 `net_device` | | 选读 |
| 17.2 `sk_buff` | | 选读（接内核网络） |
| 17.3 收发包 | | 选读 |
| 17.4 MAC / 组播 / ethtool | | 选读 |

---

## Ch18 TTY Drivers

| 节 | 重点 | 标签 |
|----|------|------|
| 18.1 tty_driver · 线路规程 | | 选读 |
| 18.2 串口 / 控制台 | | 选读 |
| 18.3 termios | | 选读 |

---

## 精读最短路径（原理补课）

```
Ch1–3 角色/模块/字符（scull）
  → Ch5 并发与锁  → Ch6 ioctl/poll
  → Ch7–8 时间与内存  → Ch9–10 硬件与中断
  → Ch11 container_of  → Ch14 LDM  → Ch15 mmap/DMA
```

总线章（12–13、16–18）按项目需要再开。

## 与 Madieu 对照提醒

| 主题 | LDD3 | Madieu |
|------|------|--------|
| 模块 / 字符 | Ch2–3 | Ch2、Ch4 |
| 工具锁/中断/定时 | Ch5、7、10 | Ch3 |
| Platform / DTS | **无** | Ch5–6 |
| I2C / SPI / GPIO | **无** | Ch7–8、14 |
| DMA | Ch15 | Ch12 |

树莓派实现 **跟 Madieu**；LDD3 只吃「为什么」。
