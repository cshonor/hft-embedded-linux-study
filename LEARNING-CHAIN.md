# HFT 学习链路 · 从知其所以然到动手实现

> **仓库：** [hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)  
> **执行顺序（定稿）：** [LEARNING-PATH-LOCKED.md](./LEARNING-PATH-LOCKED.md) — **文件夹编号 ≠ 读序**；本文保留旧编号索引导航。

```
Phase 锁定（摘要）:
  25 Harris → 02 C → 01 CSAPP → 07 TLPI + 10–12 网络
  → 04 LKD + 06 Gorman → 分叉 A 嵌入式 19–23 / B HFT 13→15→16→14→17
  → 拓展 03·05·18·00·24
```

---

## 一眼版 · 执行顺序（与锁定文档一致）

```
Phase1  25  Harris（当前）
Phase2  02  C 语言 · [c-programming](./02-c-programming/)
        01  CSAPP
Phase3  07  TLPI
        08  自制 OS（穿插）· 09 C++（穿插）
        10  PNP → 11 UNP → 12 TCP/IP
Phase4  04  LKD + 06 Gorman
Phase5A 19 → 20 → 21 → 22 → 23
Phase5B 13 Rosen → 15 SysPerf → 16 BPF → 14 DPDK → 17 HFT
Phase6  03 Hennessy · 05 ULK · 18 Rust · 00 交易 · 24 电机（兴趣）
```

**HFT 最短路径：** `25` → `02` C → `01` → `07` → `10–12` → `04`+`06` → `13`→`15`→`16`→`14`→`17`

**Harris 深度：** 黑盒为主，见 [25 学习深度](./25-Digital-Design-Harris-ARM/学习深度_时序对Linux驱动.md) · [CSAPP↔Harris](./25-Digital-Design-Harris-ARM/学习路线_CSAPP与Harris_Linux驱动.md)

---

## 为何 Phase2 是 02 C → 01 CSAPP（锁定）

| 步骤 | 作用 |
|------|------|
| **25 Harris** | 硬件黑盒词汇（延迟、时序、寄存器/FIFO）—— Phase1 |
| **02 C** | **系统级 C** — 指针、内存、链接；能读会写底层风格代码 |
| **01 CSAPP** | 汇编、栈、缓存、VM、并发 **整体图景**（主粮） |
| **03 Hennessy** | Phase6 拓展 — CSAPP 吃透后再量化加深 |
| **07 再 04** | 先用户态 API/网络，再内核入门（LKD+Gorman） |
| **09 C++** | C 过关后、PNP/HFT 前加 RAII/Modern C++ |
| **19–23 嵌入式** | Phase5A — Phase4 后开；02 过关后不重学语法 |

**理论 + 实践：** Harris 词汇 → C 抓手 → CSAPP 图景 → TLPI/网络 → 内核 → 分叉嵌入式/HFT。

---

## 文件夹 ↔ 阶段（库存标签）

| 文件夹 | 模块 | 锁定 Phase |
|--------|------|------------|
| **25** | [Harris](./25-Digital-Design-Harris-ARM/) | **1** 当前 |
| **02** | [C](./02-c-programming/) | **2** |
| **01** | CSAPP | **2** |
| **07** / **08** / **09** | TLPI · 动手 OS · C++ | **3** |
| **10–12** | PNP · UNP · TCP/IP | **3** |
| **04** + **06** | LKD · Gorman | **4** |
| **19–23** | 嵌入式支线 | **5A** |
| **13**→**15**→**16**→**14**→**17** | 内核网 · 性能 · DPDK · HFT | **5B** |
| **03** / **05** / **18** / **00** / **24** | Hennessy · ULK · Rust · 交易 · 电机 | **6** 拓展 |

---

## 内核段衔接（锁定）

```
25 Harris → 02 C → 01 CSAPP
    ↓
07 TLPI → 10–12 网络（08/09 穿插）
    ↓
04 LKD + 06 Gorman
    ↓
A: 19–23 嵌入式    |    B: 13 → 15 → 16 → 14 → 17 HFT
    ↓
Phase6: 03 · 05 ULK · 18 · 00 · 24
```

→ **[LEARNING-PATH-LOCKED.md](./LEARNING-PATH-LOCKED.md)** · [08 HFT 主次](./08-system-low-level-hands-on/HFT-AND-EMBEDDED-PRIORITY.md) · [02 C OUTLINE](./02-c-programming/OUTLINE.md) · [GitHub 仓库](https://github.com/cshonor/hft-embedded-linux-study)

---

**HFT 主线执行序号（锁定）：** `25 → 02 → 01 → 07 → 08/09穿插 → 10 → 11 → 12 → 04+06 → 13 → 15 → 16 → 14 → 17`（`03`/`05`/`18`/`00`/`24` = Phase6）

> **C++：** [09-cpp-learning-notes/](./09-cpp-learning-notes/) — Phase3 穿插，**10 PNP 前**至少过 *Effective Modern C++*
