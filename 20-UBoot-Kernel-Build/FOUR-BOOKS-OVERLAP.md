# 四本书重合度与分工（剔除 ARM 汇编）

> 对应路线：[HFT-READING-ROADMAP §嵌入式](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24) · 模块 [20 构建](./README.md) · [21 驱动](../21-Linux-Device-Driver/README.md)  
> 板卡约定：树莓派等 **5.x+ 内核** — LDD3 代码勿照搬。

## 书单

| 代号 | 书 | 模块 |
|------|-----|------|
| **A** | *Mastering Embedded Linux Programming*, 3rd（MELP / Simmonds） | [20](./mastering-embedded-linux-programming/) |
| **B** | *Embedded Linux Primer*, 2nd（Hallinan） | [20/embedded-linux-primer](./embedded-linux-primer/) · [OUTLINE](./embedded-linux-primer/OUTLINE.md) |
| **C** | *Linux Device Drivers*, 3rd（**LDD3** / 马书，2.6） | [21](../21-Linux-Device-Driver/) |
| **D** | *Linux Device Drivers Development*（Madieu，成书 4.x） | [21](../21-Linux-Device-Driver/) · [评测 MADIEU-EVAL](../21-Linux-Device-Driver/MADIEU-EVAL.md) |

---

## 核心结论

1. **C ↔ D**：主题同源（设备驱动），内核版本不同 → **概念对照、代码几乎不可互抄**，互补而非重复。  
2. **A ↔ B**：都讲嵌入式 Linux 系统架构，有少量重叠，**侧重点不同**（实操 vs 原理）。  
3. **A/B ↔ C/D**：重叠很小。A/B = 系统搭建 / 用户态 / 上层架构；C/D = **内核驱动编码**。

---

## 逐本边界

### B · Embedded Linux Primer

| | |
|--|--|
| 定位 | 嵌入式 Linux **宏观架构教科书** |
| 覆盖 | Bootloader、内核启动、内存布局、rootfs、调度基础、系统模型 |
| 特点 | 偏原理；很少驱动实操代码 |
| 与 A 重叠 | u-boot、内核编译、rootfs 构建（概念层） |

### A · MELP（Simmonds 3rd）

| | |
|--|--|
| 定位 | 嵌入式 Linux **全流程实战手册** |
| 覆盖 | Bootloader→内核裁剪→rootfs→用户态应用、调试、简单字符驱动入门 |
| 特点 | 工程导向：上电到上层应用 |
| 与 B | 启动 / 文件系统部分重复 — Primer 偏理论，MELP 偏动手 |
| 与 C/D | 仅驱动**入门**少量重合；不深入子系统 |

### C · LDD3（2.6）

| | |
|--|--|
| 定位 | 经典驱动**理论**教材 |
| 覆盖 | 字符/块驱动、中断、等待队列、sysfs、ioctl、并发锁 |
| 局限 | 接口大量过时；**不适**直接照搬到 5.x |

### D · Linux Device Drivers Development（4.x+）

| | |
|--|--|
| 定位 | **现代**驱动实战 |
| 覆盖 | 新版字符驱动、platform、设备树、probe、现代锁、`devm` |
| 与 C | 框架思想类似，API 差异大 → **对照学**，不算内容重复 |

---

## 重复内容汇总

| 关系 | 重复程度 | 怎么处理 |
|------|----------|----------|
| **A ↔ B** | **最大**：启动顺序、Bootloader、内核配置编译、rootfs | Primer 建世界观；MELP 落地；重复章可跳过 MELP 中已懂部分 |
| **A ↔ C/D** | 轻：MELP 末尾字符驱动入门 | 深入驱动交给 21；MELP 只当预告 |
| **B ↔ C/D** | 几乎无 | Primer 不讲驱动编码 |
| **C ↔ D** | 概念重合，实现不重复 | LDD3 思想 + Madieu 现代写法并行 |

---

## 推荐读序（规避无效重复）

```
B  Primer     → 先建立「系统怎么跑起来」的全局观
     ↓
A  MELP       → 理论落地：板子搭建全流程
                 （启动/rootfs 若已懂可跳，直奔应用/调试/Yocto 等）
     ↓
C + D 并行    → LDD3 吃思想；D 吃 4.x+/DT/devm（树莓派 5.x 以 D 为准）
```

| 阶段 | 做 | 不做 |
|------|----|------|
| 20 | B→A（或 A 跳过已重复的启动/rootfs） | 别把 MELP 当驱动主书 |
| 21 | C 思想 + D 现代 API | **禁止**把 LDD3 样例当 5.x 模板硬抄 |

### 树莓派 5 / 现代内核

内核属 **5.x+** → LDD3 用来理解驱动模型；实现与设备树跟 **D**（及 [22 DT](../22-Device-Tree-Study/)）。

---

## 与仓库旧顺序的差异

旧路线图曾写「MELP → Primer」。本文件定为 **Primer → MELP**（先原理后实操）。  
[HFT-READING-ROADMAP](../HFT-READING-ROADMAP.md) / [20 README](./README.md) 已按此对齐。
