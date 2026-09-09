# 09 · Linux 设备驱动：在 Pi 5 上跑通一个驱动

> **定位：** **内核态** — 补齐 HFT 链里「只写用户态」的缺口。
> **组织方式：** 目录是**要写出什么驱动**，不是书的章节。书降级为工具书，见 [`_refs/BOOK-MAP.md`](./_refs/BOOK-MAP.md)。
> **动手不在这里：** 实际命令与板上结果放 [`projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md`](../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md) 的 Phase C。
> **前置：** [08 构建链](../08-embedded-boot-build) · [05 内核](../05-linux-kernel) · [01 C](../01-c-language)

---

## 任务节点

| # | 节点 | 交付物 | 对应动手 |
|---|------|--------|----------|
| 01 | [hello-module](./01-hello-module/) | 自写 `.ko` 能 `insmod`，`dmesg` 有输出 | P5 · 热身 |
| 02 | [char-device](./02-char-device/) | `open/read/ioctl` 从用户态落到驱动 | P5 · C1 |
| 03 | [platform-dt](./03-platform-dt/) | **platform 驱动 + 设备树，资源全从 DT 解析** | P5 · C3 |
| 04 | [gpio-i2c-spi](./04-gpio-i2c-spi/) | 真实传感器读到数据 | P5 · C2 |
| 05 | [irq-locking](./05-irq-locking/) | 懂上下文约束，锁选型正确 | — |
| 06 | [dma-mmap](./06-dma-mmap/) | `mmap` 零拷贝读数据 | — |

**03 是核心** —— SoC 上的外设不可枚举，全靠设备树描述，这是现代 ARM 驱动的主干。

---

## 两本书怎么用

| 书 | 内核 | 怎么用 |
|----|------|--------|
| *Linux Device Drivers Development*（Madieu） | 4.1–4.13 | **主查这本**：现代 API、有 DTS |
| *Linux Device Drivers*, 3rd（LDD3） | **2.6.10** | **只补原理**：锁/DMA/内存/并发的思想，**代码勿抄** |

LDD3 的 18 章里值得回头精读的只有 **Ch3 / 5 / 6 / 9 / 10 / 15**；Ch12 PCI、Ch13 USB、Ch16 block、Ch17 net、Ch18 tty 主线用不上。详见 [`_refs/BOOK-MAP.md`](./_refs/BOOK-MAP.md)。

**书目原则：** 全外文，不用国产驱动书。

---

## 与 HFT 链的关系

| HFT 已学 | 驱动侧延伸 |
|----------|-----------|
| 用户态 `epoll` / `mmap` | 内核 `poll`/`wait_queue` · `remap_pfn_range` |
| 无锁 / spinlock 概念 | 内核 `spinlock_t` · 中断上下文 |
| [12.5 现代网络](../12.5-modern-networking) | NAPI 就是「中断 + 轮询」混合 |
| [13 DPDK](../13-dpdk) | UIO/VFIO **旁路** vs 内核驱动**标准路径** |

**为什么 HFT 要学驱动：** 不是为了写驱动，是为了知道**用户态那层抽象下面到底发生了什么**——一次 `read()` 的代价、一次中断的抖动来源、旁路技术到底绕过了什么。

---

## 技能清单

| 技能 | 节点 |
|------|------|
| `module_init/exit` · `printk` · 内核 Makefile | 01 |
| 主次设备号 · `file_operations` · `copy_to_user` | 02 |
| `platform_driver` · `compatible` · `devm_*` · `ioremap` | 03 |
| gpiod · `i2c_driver` · `spi_driver` · regmap | 04 |
| `request_threaded_irq` · spinlock vs mutex | 05 |
| `dma_alloc_coherent` · `dma_map_single` · `remap_pfn_range` | 06 |

---

## 设备树官方文档（比书权威）

| 文档 | 用途 |
|------|------|
| [Usage Model](https://docs.kernel.org/devicetree/usage-model.html) | `compatible` · 匹配 · FDT/DTB 启动链 |
| [Devicetree Spec](https://devicetree-specification.readthedocs.io/en/latest/usage-model.html) | 语法 · `reg`/`interrupts` · phandle |
| [Bindings 索引](https://docs.kernel.org/devicetree/bindings/index.html) | 查外设 `compatible` |
| [Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html) | DT overlay |

---

## Pi 5 特别注意

- 外设经 **RP1**（PCIe 南桥），引脚/寄存器叙事与 Pi 4 不同 → **别照抄 Pi 4 教程**
- 内核 6.x：LDD3（2.6）的 API 大面积失效，**思想可读，代码勿抄**
- 看驱动源码需要标准 C + 少量 GNU 扩展：[速查](../01-c-language/05-embedded-kernel-practice/01-ch1-gnu-c-basics/DRIVER-GNU-C-CHEATSHEET.md)

---

## 验收（模块级）

- [ ] 写过最小字符设备（ioctl + read/write）
- [ ] **能默出用户态 `open()` 如何落到驱动的 `open`**
- [ ] 知道硬中断里不能 sleep，能说出 3 个禁用的函数
- [ ] Pi 5 上改过 DTS 并匹配 platform / I2C 驱动成功
- [ ] 能解释 DTB 从哪来、内核用来干什么
- [ ] 做过一次 `mmap`，用户态零拷贝读到数据

---

## 进度

- [x] 去书本化重构：40 章书目录 → 6 个任务节点
- [x] 01-hello-module（3 篇）
- [x] 03-platform-dt（2 篇）
- [x] 04-gpio-i2c-spi（2 篇）
- [x] 06-dma-mmap（4 篇）
- [ ] 02-char-device
- [ ] 05-irq-locking
