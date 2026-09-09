# 书 → 任务节点 映射表

> **本模块已经去书本化。** 目录是「要写出什么驱动」，不是「书的第几章」。
> 书降级为**工具书**：写某个驱动卡住了，来这张表查该翻哪一章。
> LDD3（18 章）/ Madieu（22 章）原骨架已归档到 `_archive/`，只作溯源。

---

## 两本书的分工

| 代号 | 书 | 内核基准 | 怎么用 |
|------|-----|----------|--------|
| **D** | *Linux Device Drivers Development* — Madieu | 4.1–4.13 | **主查这本**：现代 API、有 DTS、代码能改改就跑 |
| **C** | *Linux Device Drivers*, 3rd（LDD3） | **2.6.10** | **只补原理**：锁 / 并发 / DMA / 内存 / LDM 的思想。**代码勿抄** |

> LDD3 的价值在"为什么这么设计"，不在"照着敲"。Pi 5 是 6.x 内核，LDD3 的 API 大面积变了。
> 详评：[LDD3-EVAL.md](./LDD3-EVAL.md) · [MADIEU-EVAL.md](./MADIEU-EVAL.md)

---

## 按任务节点查书

| 任务节点 | 主查 | 原理补课 |
|----------|------|----------|
| [01-hello-module](../01-hello-module/) | **D** Ch1–3（模块、内核设施与助手宏） | **C** Ch1–2（什么是驱动、模块装载）· Ch11（内核数据类型，可移植性） |
| [02-char-device](../02-char-device/) | **D** Ch4 | **C** Ch3（字符设备，scull）· Ch6（高级字符操作：ioctl / 阻塞 / poll） |
| [03-platform-dt](../03-platform-dt/) | **D** Ch5（platform）· Ch6（设备树） | **C** Ch14（Linux 设备模型）· **D** Ch13 |
| [04-gpio-i2c-spi](../04-gpio-i2c-spi/) | **D** Ch7（I2C）· Ch8（SPI）· Ch14（pinctrl/GPIO）· Ch15（GPIO 控制器） | **C** Ch9（与硬件通信：I/O 端口与内存） |
| [05-irq-locking](../05-irq-locking/) | **D** Ch16（高级中断管理）· Ch3（内核锁助手） | **C** Ch5（并发与竞态）· Ch10（中断处理）· Ch7（延迟执行：workqueue / tasklet） |
| [06-dma-mmap](../06-dma-mmap/) | **D** Ch11（内核内存）· Ch12（DMA） | **C** Ch15（内存映射与 DMA）· Ch8（分配内存） |

---

## 按需拓展（主线之外，用到再查）

| 想写/想搞清 | 查 |
|-------------|-----|
| 寄存器封装（别再手撕 `readl/writel`） | **D** Ch9 — regmap API |
| 传感器走标准框架（ADC/IMU） | **D** Ch10 — IIO |
| 网卡驱动 / 与 DPDK 的分界 | **D** Ch22 · **C** Ch17 |
| input（按键/触摸）、RTC、PWM、regulator、framebuffer | **D** Ch17–21 |
| 调试技巧 | **C** Ch4（内核调试） |

---

## 明确**不读**的（LDD3 里的坑位）

这几章对 HFT + 嵌入式网关主线基本无用，硬啃是浪费时间：

| LDD3 章 | 为什么不读 |
|---------|-----------|
| Ch12 PCI 驱动 | 主线用不到；Pi 5 上 RP1 虽挂 PCIe，但那是官方驱动的事 |
| Ch13 USB 驱动 | 同上，用不到 |
| Ch16 块设备驱动 | 与 HFT/嵌入式主线无关 |
| Ch17 网络驱动 | 除非要写网卡；DPDK 旁路路线（[13-dpdk](../../13-dpdk/)）更贴近 HFT |
| Ch18 TTY 驱动 | 用不到 |

LDD3 的 18 章里，**真正值得回头精读的是 Ch3 / 5 / 6 / 9 / 10 / 15**，其余按需。

---

## 官方设备树文档（与节点 03 并行，比书更权威）

| # | 文档 | 读什么 |
|---|------|--------|
| 1 | [Linux and the Devicetree（Usage Model）](https://docs.kernel.org/devicetree/usage-model.html) | `compatible` · platform 匹配 · FDT/DTB 启动链 |
| 2 | [Devicetree Spec — Usage](https://devicetree-specification.readthedocs.io/en/latest/usage-model.html) | DTS 语法 · `reg` / `interrupts` · phandle |
| 3 | [Bindings 索引](https://docs.kernel.org/devicetree/bindings/index.html) | 查外设 `compatible` |
| 选读 | [Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html) | DT overlay（Pi 5 上常用） |

---

## 归档

`_archive/` 保留原书章节骨架（LDD3 18 章 / Madieu 22 章），仅供溯源：

- `_archive/classic-driver-theory/` — LDD3
- `_archive/modern-driver-practice/` — Madieu
- 完整章节大纲：`OUTLINE-LDD3.md` · `OUTLINE-MADIEU.md`
