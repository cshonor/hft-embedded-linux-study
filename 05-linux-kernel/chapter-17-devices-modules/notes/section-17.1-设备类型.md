## ① 设备类型 · Device Types

Unix/Linux 将设备分为几类（+ 扩展）：

| 类型 | 访问方式 | 特点 | 示例 |
|------|----------|------|------|
| **块设备（block）** | **块设备节点** · 常挂载 FS | **固定块大小** · **可随机寻址（seek）** | 硬盘、SSD、蓝光、闪存 |
| **字符设备（character）** | **字符设备节点** | **字节流** · **不可寻址** | 键盘、鼠标、串口 |
| **网络设备（network）** | **非设备节点** — **`socket` API** | 物理网卡 + 协议栈 | 以太网卡 |

#### 扩展类型

| 类型 | 说明 |
|------|------|
| **杂项设备（misc）** | **字符设备的简化** — 主设备号固定 10，只用一个次设备号；省得申请主设备号 |
| **伪设备（pseudo）** | **内核虚拟功能** — 非物理硬件 |

| 伪设备示例 | 作用 |
|------------|------|
| **`/dev/null`** | 黑洞 — 写入丢弃，读取返回 EOF |
| **`/dev/zero`** | 读出来全是 0（分配大块零页时常用） |
| **`/dev/full`** | 写入永远返回 `ENOSPC`（测试错误处理） |
| **`/dev/random` / `urandom`** | 随机数 |
| **`/dev/ptmx`** | 伪终端主设备（ssh/docker exec 的背后） |
| **`/dev/loop*`** | 把文件当成块设备挂（镜像文件） |

---

### 设备号：dev_t 的位布局

```c
/* include/linux/kdev_t.h — v6.6 原文 */
#define MINORBITS	20                        /* :7  */
#define MINORMASK	((1U << MINORBITS) - 1)   /* :8  */
#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))   /* :10 */
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))            /* :12 */
```

```
 dev_t（32 位）
┌──────────────┬──────────────────────────────────┐
│  主设备号 12 │        次设备号 20 位             │
│ （驱动/类型）│      （同类设备的第几个）          │
└──────────────┴──────────────────────────────────┘
  8192 种            约 100 万个
```

| 注册方式 | 用法 | 现状 |
|---------|------|------|
| `register_chrdev_region(dev, count, name)` | **静态**申请指定主设备号 | 易冲突，不推荐 |
| **`alloc_chrdev_region(&dev, baseminor, count, name)`** | **动态**分配（推荐） | 现代驱动的标准做法 |
| `misc_register(&miscdev)` | misc 设备专用，主设备号固定 **10** | 单实例小驱动最省事 |

> **主次设备号的唯一作用**是"在 `/dev` 下定位到正确的驱动"。
> 看到 `crw-rw-rw- 1 root root 1, 3 ...` 里的 `1, 3` 就是 major=1、minor=3 → `/dev/null`。

---

### 「一切皆文件」的边界

```
「一切皆文件」：
  block/char ──► /dev/sda、/dev/ttyS0 ──► open/read/write/poll/mmap
  network    ──► 打破该原则 ──► socket()（不走 VFS 设备节点）
```

| 为什么网络设备例外 | 说明 |
|------------------|------|
| **没有"可以读的字节流"** | 网络的最小单位是**带地址的报文**，不是字节序列 |
| **有独立的命名空间** | 地址是 (IP, port, proto)，装不进路径名 |
| **性能** | 多一次 VFS 层拷贝/查找，在 10Gbps+ 上是不可接受的 |

> 但网络设备**并没有完全脱离内核对象体系**——它在 sysfs 下有 `/sys/class/net/eth0/`，
> 只是**不走 `/dev` 节点 + VFS 读写**。所以更准确的说法是：
> **网络设备有对象模型，无文件接口。**

### HFT 常用的特殊设备

| 设备 | 用途 | 关键点 |
|------|------|--------|
| **`/dev/vfio/vfio`** | **VFIO** —— 安全地**把 PCI 设备交给用户态**（DPDK 首选） | 有 **IOMMU** 保护，用户态 DMA 被限制在自己进程的映射内 |
| **`/dev/uio*`** | **UIO** —— 老式用户态驱动框架（igb_uio） | **无 IOMMU 保护**，用户态可 DMA 到任意物理地址，**有安全风险** |
| **`/dev/hugepages`（hugetlbfs）** | 大页内存，DPDK 巨页池的基础 | 需 `echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages` |
| **`/dev/shm`** | tmpfs 共享内存 | 进程间零拷贝行情传递 |
| **`/dev/cpu/*/msr`** | 读 MSR（如 `IA32_TSC`、性能计数器） | 读 TSC 做时间戳时的替代方案 |

> **UIO 已被 VFIO 取代**（DPDK 官方推荐 VFIO）。核心差别是 **IOMMU**：
> UIO 把设备的 DMA 能力直接交给用户态进程，一个 bug 或恶意程序就能 DMA 写穿整个物理内存；
> VFIO 通过 IOMMU 把设备的 DMA 地址空间**限制在该进程被授予的范围内**，
> 性能开销极小（地址转换硬件做），安全性天差地别。

→ [Ch 1](../../chapter-01-intro/) · [Ch 13](../../chapter-13-vfs/) · [Ch 14](../../chapter-14-block-io/)

**HFT：** 行情路径走 **网卡 + socket/DPDK**；配置/调优常读 **`/sys/class/net/...`**。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 字符设备、块设备、网络设备的区别？HFT 系统中各有什么？

<details><summary>答案</summary>

字符设备：按字节流访问，无缓冲（串口/传感器/GPU）。块设备：按块随机访问，有 page cache（磁盘/SSD/NVMe）。网络设备：无 /dev 节点，通过 socket 接口（网卡）。HFT：网卡=网络设备（行情/订单），NVMe=块设备（历史数据/日志），FPGA=字符设备（自定义加速）。网络设备是最特殊的——不走 VFS。

</details>

**Q2.** `/dev/vfio/vfio` 和 `/dev/uio0` 都能让用户态驱动网卡，为什么现代 DPDK 推荐 VFIO？

<details><summary>答案</summary>

核心差别是 **IOMMU 保护**。

- **UIO（`drivers/uio/uio.c`）**：它做的事情本质上是"把设备的寄存器映射给用户态 + 让用户态处理中断"。但设备的 **DMA 能力也被一并交了出去** —— 用户态程序可以配置网卡去 DMA 读写**任意物理地址**，包括内核代码、其他进程的内存。这是**没有任何硬件约束**的，纯粹靠程序自觉。
- **VFIO（`drivers/vfio/vfio_main.c` + `vfio_iommu_type1.c`）**：在把设备交给用户态之前，先通过 **IOMMU** 为这个进程建立一张**独立的 DMA 地址转换表**。设备发出的 DMA 地址会被 IOMMU 翻译并**校验权限**，越界访问直接被硬件拒绝并报错。

所以：

| | UIO | VFIO |
|---|-----|------|
| DMA 安全 | ❌ 无约束，可写穿物理内存 | ✅ IOMMU 硬件隔离 |
| 性能开销 | 0 | **几乎 0**（地址转换由 IOMMU 硬件完成，不进内核） |
| 中断 | 支持 | 支持 |
| 设备热插拔 / 多设备 | 弱 | 好（有 container / group 概念） |
| DPDK 支持 | 保留（igb_uio） | **默认推荐** |

**对 HFT 的实际意义**：低延迟和安全性在这里**不冲突**——IOMMU 的转换是硬件并行做的，不是软件查表，实测开销几乎测不出来。所以没有理由再为了"省 IOMMU"去用 UIO。

使用前需要：BIOS 打开 `VT-d` / `AMD-Vi`，内核启动参数带 `intel_iommu=on`（或 `amd_iommu=on`），然后 `/sys/bus/pci/devices/0000:xx:00.0/driver/unbind` + `vfio-bind`。

</details>

**Q3.** 字符设备的 `register_chrdev_region` 和 `alloc_chrdev_region` 该怎么选？为什么现代驱动都用后者？

<details><summary>答案</summary>

选 **`alloc_chrdev_region()`**（动态分配），除非你有历史包袱。

差别在于**主设备号从哪来**：
- `register_chrdev_region(dev, count, name)`：你**指定**一个主设备号（如 240），内核检查它是否空闲。问题是 240 在你这台机器上空闲，到了客户机器上可能已经被占用 → **驱动加载失败**。
- `alloc_chrdev_region(&dev, baseminor, count, name)`：内核**挑一个空闲的**主设备号返回给你，你再拿它去 `cdev_add()`。永不冲突。

配套做法是：**不要指望 `/dev/xxx` 节点手工 `mknod` 创建**，而是在驱动里调用
`device_create(class, parent, devt, drvdata, "mydev%d", minor)` ——
这会走设备模型，通过 **uevent 通知 udev**，由 udev 自动创建 `/dev/mydev0` 节点（见 17.4）。
用户态永远不需要知道主次设备号是多少。

完整的现代字符设备初始化顺序是：
```c
alloc_chrdev_region(&dev, 0, count, "mydev");   /* 1. 动态拿设备号 */
cdev_init(&my_cdev, &fops);                      /* 2. 绑定操作表  */
cdev_add(&my_cdev, dev, count);                  /* 3. 注册到内核  */
cls = class_create("mydev");                     /* 4. 建 class    */
device_create(cls, NULL, dev, NULL, "mydev%d", minor);  /* 5. 触发 uevent 建节点 */
```
第 4、5 步就是"设备模型"这一章讲的机制在字符设备上的落地——**没有它们，`/dev` 下不会出现节点**。

</details>

</details>
---
