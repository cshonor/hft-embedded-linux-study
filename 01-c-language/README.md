# 02 · C 语言 · 系统级编程

**文件夹 `02`** · [README](../README.md) · [OUTLINE](./OUTLINE.md)

> **定位：** 面向 **底层 / Linux 内核** 的经典 C 书单（五书 + `code`）。  
> 路线：**K&R（C89）→ 进阶标准 C → GNU C → 内核**。  
> 上游 [01 CSAPP](../02-computer-systems/)；下游 [04 LKD](../05-linux-kernel/)（`-std=gnu11` / GNU C 见 [Ch2 §2.4](../05-linux-kernel/chapter-02-getting-started/notes/section-2.4-内核开发的特点.md)）。  
> **09 C++** 是后续加 RAII，不是跳过 C。

---

## 为什么学 C

- Linux 内核、内核模块、网络协议栈以 **C + GNU 扩展** 为主
- DPDK、高性能网卡旁路、用户态网络栈依赖 C 与底层内存模型
- 与 C++ 主线配合：C++ 做业务与框架，C 啃内核与数据面

---

## 目录一一对应

| 目录 | 书 | 一句话 |
|------|-----|--------|
| [01-Primer-K-and-R-C](./01-Primer-K-and-R-C/) | 《C 程序设计语言》**K&R 第2版** | **= C89** 奠基（≠ C99/C11） |
| [02-Pointers-on-C](./02-Pointers-on-C/) | 《C 和指针》· Kenneth Reek · *Pointers on C* | 指针 / 数组 / 内存模型（内核重中之重） |
| [03-Advanced-Expert-C-Programming](./03-Advanced-Expert-C-Programming/) | 《C 专家编程》（鱼封面） | 内存布局、段、链接器、ANSI 历史 |
| [04-Kernel-Prep-Embedded-C-Self-Cultivation](./04-Kernel-Prep-Embedded-C-Self-Cultivation/) | 《嵌入式 C 语言自我修养》· 王利涛 | ✅ **GNU C**：`__attribute__` / `typeof` / 内嵌汇编 / ELF |
| [05-Reference-C-Traps-and-Pitfalls](./05-Reference-C-Traps-and-Pitfalls/) | 《C 陷阱与缺陷》 | 优先级、数组指针、链接、UB 避坑（工具书，遇坑再查） |
| [code](./code/) | 配套示例 | 练习与索引 |

> **纠正常见书名混淆：** `02-Pointers-on-C` 是 Reek 的 *Pointers on C*（中译《C 和指针》），**不是** O'Reilly 的 *Understanding and Using C Pointers*（《C 指针：理解与运用》）。

来源副本说明 → [README.external.md](./README.external.md)

> **CSAPP 实验在 [02-computer-systems/code](../02-computer-systems/code/)** — **不在 02 重复**。

---

## 推荐阅读顺序（Linux 内核目标）

```
01 K&R（C89 打底）
 → 02 C 和指针（吃透指针）
 → 03 C 专家编程（内存 / 链接，只读 ch05–ch07）
 → 04 嵌入式 C 自我修养（GNU C 收尾，内核桥）
 → 05 C 陷阱与缺陷（工具书，遇坑再查）
 → 04 LKD / 驱动 / DPDK
```

| 阶段 | 做什么 |
|------|--------|
| **1–3** | **标准 C 范畴**（C89 基底 + 指针/内存/链接进阶） |
| **4** | **专门补 GCC/GNU C 扩展** — 打通「标准 C 书 → 读得懂内核」的鸿沟 |
| **5** | **工具书** — 避坑手册，遇到问题再查，不必逐章通读 |

**重点收尾是 04**：标准 C 教材不讲、`typeof` / 语句表达式 / `__attribute__` / 内嵌汇编等内核天天用的东西，主要在这里补齐。  
对照清单也在 [LKD §2.4「K&R 有 / 内核缺」](../05-linux-kernel/chapter-02-getting-started/notes/section-2.4-内核开发的特点.md)。

完整裁剪与验收 → [OUTLINE.md](./OUTLINE.md)

### 阅读优先级（精读 / 略读 / 跳过）

针对 **HFT / 内核方向**，5 本书的投入策略（2026-08 讨论确定）：

| 书 | 策略 | 理由 |
|----|------|------|
| 01 K&R | 🔴 精读 | C89 奠基，校准语法。注意看 **第 2 版（1989, ANSI C89）**，别看 1978 第一版（无函数原型）。薄而精，例题经典，但只覆盖 C89 |
| 02 C 和指针 | 🔴 精读 | 指针 / 数组 / 内存模型 / ABI —— 内核向重中之重，结构体对齐靠它 |
| 03 C 专家编程 | 🟡 **只读 ch05–ch07** | 只读 **链接器 + 段 + 内存布局**：ch05 链接 / ch06 运行时数据结构（a.out、段、栈帧）/ ch07 内存探险（虚拟内存、数据段、堆）。其余跳（ANSI 历史、优先级表与 05 重复） |
| 04 嵌入式 C 自我修养 | 🔴 必读 | GNU C 扩展（`typeof` / `__attribute__` / 内嵌汇编 / ELF）—— 标准到内核的桥，**不能省** |
| 05 C 陷阱与缺陷 | 🟡 略读 / 工具书 | 薄，快速翻记避坑清单（优先级、数组退化、链接、UB），或遇到再查。**不必逐章** |

> **关键提醒**：别在 01–03 上磨太久。K&R + Reek 够你读懂指针和结构体，03 专家编程只读 ch05–ch07 即可。真正的分水岭是 **04（GNU C）** —— 标准书不讲、内核天天用的东西主要在这里补齐。03 的 ch05–ch07 与 04 的 ch06/ch08/ch10 已补"常见陷阱 + 自测题"段落，精读时可用。05 是工具书，遇到坑再翻。

### 学习进度

- [ ] 01 K&R
- [ ] 02 C 和指针
- [ ] 03 C 专家编程
- [ ] 04 嵌入式 C 语言自我修养
- [ ] 05 C 陷阱与缺陷（工具书）

---

## 在主线中的位置

| 上游 | 本模块 | 下游 |
|------|--------|------|
| [01 CSAPP](../02-computer-systems/) | **指针、内存、GNU-C** | [19 Hennessy](../18-computer-architecture/) → [04–07](../05-linux-kernel/) → [08 MikanOS](../projects/P9-os-from-scratch/mikanos/) |

**下一步：** 打开 **[01-Primer-K-and-R-C](./01-Primer-K-and-R-C/)**；若已过标准 C，直奔 **[04 · ch06 GNU C](./04-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/)** 再进 LKD。
