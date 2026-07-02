# HFT / 嵌入式主线 · 学什么、先搁置什么

> **结论（定死）：** **`01 CSAPP → 02 C → 03 Hennessy → 04–07 内核/TLPI → 08/01 MikanOS → … → 17 HFT`**  
> [02 30 天 OS](./02-30days-os/) **彻底后置**。详 [LEARNING-CHAIN](../../LEARNING-CHAIN.md)

---

## 一、C 语言放在哪？

| 顺序 | 模块 | 作用 |
|------|------|------|
| **01** | CSAPP | 硬件 + 机器级程序 **图景** |
| **02** | **[C 语言](../../02-c-programming/)** | **系统级 C** — 指针、内存、链接（K&R · Pointers on C） |
| **03** | Hennessy | CPU/缓存/ILP **量化** |
| **04–07** | LKD · ULK · Gorman · TLPI | 内核与用户态 — **主体是 C** |
| **08/01** | MikanOS | UEFI/64 位 OS 动手 |
| **09** | C++ | muduo/HFT — **C 过关后再开** |
| **19–24** | 嵌入式 Linux · 飞控 | **C 与硬件/内核直接交互** — 02 过关后支线复用，不重学语法 |

**不要跳过 02：** MikanOS、LKD、DPDK、**嵌入式驱动/飞控（19–24）** 都假设你能 **读写的 C** 过关；CSAPP alone 不够写系统代码或与寄存器对话。

---

## 二、MikanOS vs 30 天 OS

| | **08/01 MikanOS** | **08/02 30 天** |
|--|-------------------|-----------------|
| HFT 路线 | **🔴 主攻** | **⚪ 可不学** |
| 架构 | UEFI + x86-64 | BIOS 16 位 |

---

## 三、HFT 主线（文件夹编号）

```text
01 CSAPP → 02 C → 03 Hennessy
→ 04–07 内核/TLPI
→ 08/01 MikanOS
→ 09 C++ → 10–14 网络/DPDK
→ 15–16 SysPerf/BPF
→ 17 HFT → 18 Rust
```

**30 天 haribote：** 后置休闲；16 位 IPL **不必死磕**。

---

## 四、汇编学什么

| 类型 | 何时 |
|------|------|
| **x86-64（CSAPP）** | 01 + 02 C 并行 |
| **ARM64** | 嵌入式 19+ 支线 |

---

## 五、一句话

**02 C 是 CSAPP 与 Hennessy/内核之间的桥** — 嵌入式底层开发的「通用母语」；**08 MikanOS** 是 OS 动手主线；30 天可不学。

---

## 相关

| 文档 | 用途 |
|------|------|
| [02-c-programming/](../../02-c-programming/) | **C 主线模块** |
| [08/01-mikan-os/](./01-mikan-os/) | MikanOS |
| [LEARNING-CHAIN.md](../../LEARNING-CHAIN.md) | 全链 `00`–`18` |
