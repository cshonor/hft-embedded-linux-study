# 第4章 硬件选型与服务器配置

> **BIOS 确定性 · NIC · FPGA · NUMA**

← 总览：[chapter-01 §2](./chapter-01-高频交易基础与生态.md#2-硬件与操作系统优化) · [§6 FPGA](./chapter-01-高频交易基础与生态.md#6-fpga纳秒级)

---

## 1. 服务器与 BIOS

| 配置 | 建议 |
|------|------|
| **CPU** | 高主频、少 NUMA 跳；关键线程 **同 socket** |
| **HT** | **关** |
| **Turbo / C-states** | **关**（低 jitter） |
| **NUMA** | 内存、NIC、绑核 **同一 node** |

→ [chapter-05 OS 调优](./chapter-05-操作系统内核极致调优.md)

---

## 2. 网卡

| 类型 | 场景 |
|------|------|
| **Solarflare（现 AMD/Xilinx）** | OpenOnload **kernel bypass** |
| **Intel + DPDK** | 通用 **PMD 轮询** |
| **硬件时间戳** | T2T 测量 **必备** |

---

## 3. FPGA 加速

当软件 **1–5 μs** 不足：

| 可下沉 FPGA | 说明 |
|-------------|------|
| **MD 解码** | 比特流 **流水线** |
| **轻量策略** | 固定规则 **trigger** |
| **TOE / 自定义 MAC** | 跳过 GPP 协议栈 |

| 优势 | **无 OS · 无调度** — T2T **<500 ns** 可达 |
|------|------------------------------------------|
| 代价 | 开发周期、灵活性、验证成本 |

→ [14-DPDK Intro NFV/OVS](../14-DPDK-Low-Latency-Network/01-Intro-Book/)（软硬件协同语境）

---

## 4. 栈空间与第三方库（经验）

复杂解码库（图像/字体/协议）可能 **栈需求 >32KiB** — HFT 嵌入 **stb/压缩库** 时同理：**压测栈 usage**。

（OS 教学见 [MikanOS Ch28/Ch30 栈扩容](../09-system-low-level-hands-on/02-mikan-os/chapter-30-extra-apps/) — 工程原则相通）
