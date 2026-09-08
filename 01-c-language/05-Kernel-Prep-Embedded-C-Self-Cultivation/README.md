# 《嵌入式 C 语言自我修养》

**从芯片、编译器到操作系统** · 王利涛

> **第 5 本书** · GNU C 扩展（标准 → 内核的桥）
> **六章结构**（2026-09-08 重组）：**CH1–CH2 工具 → CH3–CH4 战场 → CH5–CH6 边界**

---

## 怎么读这本书

这本书**不按原书章节顺序读**。硬件/体系结构别的书讲得更深（CSAPP），重复读没意义。全书按一个逻辑推进：

```text
 工具              战场                边界
 ┌────────┐    ┌──────────────┐    ┌────────────┐
 │ CH1 语法 │ -> │ CH3 裸机/驱动  │ -> │ CH5 跨平台  │
 │ CH2 语义 │ -> │ CH4 内核模块   │ -> │ CH6 工具链  │
 └────────┘    └──────────────┘    └────────────┘
   有什么            在哪用             走多远
```

**CH1/CH2 必须按顺序读**（CH1 建立语感，CH2 全是坑）。**CH3/CH4 可以二选一**：做裸机走 CH3，看内核走 CH4。**CH5/CH6 按需查阅**。

---

## 六章一览

| 章 | 目录 | 一句话 | 状态 |
|----|------|--------|------|
| **CH1** | [GNU C 基础扩展语法](./01-ch1-gnu-c-basics/) | **写在源码里看得懂的扩展**：`typeof`、语句表达式、柔性数组、指定初始化 | 6 篇精写 129 KB |
| **CH2** | [GNU C 高级特性](./02-ch2-gnu-c-advanced/) | **源码看不见却真实发生**：`__attribute__`、`packed`、`weak`、`inline asm` | 5 篇精写 135 KB |
| **CH3** | [嵌入式驱动中的 GNU C 实战](./03-ch3-embedded-driver/) | **同样是 C，写寄存器时为什么要这样**：`volatile`、屏障、位操作、ISR 共享 | 10.8 已写 24 KB |
| **CH4** | [内核模块与子系统的 GNU C 应用](./04-ch4-kernel-module/) | **工具放回它被用的地方**：`initcall`、`container_of`、`file_operations` | 骨架 6 KB，待补写 |
| **CH5** | [基于 GNU C 的跨平台模块化设计](./05-ch5-portable-modular/) | **一份代码换个平台还对吗**：数据表示、OOP in C、头文件与模块划分 | 86 篇 294 KB |
| **CH6** | [GNU C 编译链定制](./06-ch6-toolchain-custom/) | **代码之外决定它跑在哪**：链接脚本、静态/动态库、交叉编译 | 70 篇 190 KB |

> **CH4 是六章里唯一需要大量补写的**——它的价值在于把 CH1/CH2 的扩展放回内核真实代码里看，补写计划见 [CH4 README](./04-ch4-kernel-module/)。

## 附录（不在六章主线内，按需查阅）

| 目录 | 内容 | 什么时候看 |
|------|------|-----------|
| [附录 A · OS 通识](./90-ref-os/) | 进程/线程/调度/系统调用/文件系统/MMU | 别的书讲得更深，只是备查 |
| [附录 B · 内存与堆栈](./91-ref-memory/) | 函数调用栈、堆管理、mmap、泄漏与踩踏检测 | 排查内存问题时 |
| [附录 C · ARM 汇编残留](./92-ref-arm-asm/) | 原第 3 章剩下的 AArch64 选读 | 极少用 |

> 原第 2 章（计算机体系结构 48 篇）、原 3.1–3.5 / 3.9（ARM 指令 25 篇）**共 73 篇已物理删除，文件夹也一并删掉**。
> 恢复：`git log --diff-filter=D --name-only -- 01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/`

## 六章 ↔ 原书对照

「这一节原来在第几章」——如果只是对不上号，查这里：

| 原书 | 归属 |
|------|------|
| 第 1 章 工具链 | [CH6 · 1 工具链](./06-ch6-toolchain-custom/1-toolchain/) |
| 第 2 章 体系结构 | 🗑️ **已删**（CSAPP 更深） |
| 3.1–3.5 / 3.9 ARM 指令 | 🗑️ **已删** |
| 3.6 C/汇编混合编程 | [CH2](./02-ch2-gnu-c-advanced/3.6-mixed-programming/)（本质是 GNU 扩展） |
| 3.7 GNU ARM 工具链 | [CH6](./06-ch6-toolchain-custom/3.7-gnu-arm/) |
| 3.8 AArch64 | [附录 C](./92-ref-arm-asm/) |
| 第 4 章 编译链接 | [CH6 · 2 编译与链接](./06-ch6-toolchain-custom/2-compile-and-link/) |
| 4.10 / 4.11 / 4.12 内核模块与 U-Boot | [CH4](./04-ch4-kernel-module/) |
| 4.14 链接脚本 | [CH6](./06-ch6-toolchain-custom/4.14-链接脚本.md) |
| 第 5 章 内存堆栈 | [附录 B](./91-ref-memory/) |
| **第 6 章 GNU C 扩展** | **[CH1](./01-ch1-gnu-c-basics/) + [CH2](./02-ch2-gnu-c-advanced/)** ← 本书核心 |
| 第 7 章 数据与指针 | [CH5](./05-ch5-portable-modular/7-data-and-pointers/) |
| 第 8 章 OOP in C | [CH5](./05-ch5-portable-modular/8-oop-in-c/) |
| 第 9 章 模块化 | [CH5](./05-ch5-portable-modular/9-modular-c/) |
| 10.1 / 10.3 / 10.8 嵌入式 | [CH3](./03-ch3-embedded-driver/) |
| 第 10 章 其余 OS 通识 | [附录 A](./90-ref-os/) |

## 定位

Linux 内核、内核模块、DPDK 代码大量依赖 GNU C 扩展；**标准 C 教材完全不讲这些**。读 LKD、虚拟内存、内核网络源码前必须先过 CH1 + CH2。

CH3/CH4 解决「知道了工具，但不知道什么场合该用」；CH5/CH6 决定这套东西能走多远。

## 学习进度

- [x] **CH1**：6.3 语句表达式 ✅ · 6.4 typeof/container_of ✅ · 6.5 柔性数组 ✅ · 6.10 inline ✅ · 6.12 变参宏 ✅ · 6.2 指定初始化 ✅
- [ ] **CH1**：6.1 C 标准与 gnu11（收官，最后一节）
- [x] **CH2**：6.6 `__attribute__` ✅ · 6.7 aligned/packed ✅ · 6.9 weak/alias ✅
- [ ] **CH2**：6.8 format · 6.11 内建函数 · 3.6 内联汇编
- [x] **CH3**：10.8 嵌入式 C 开门（24 KB，WSL 实测）
- [ ] **CH3**：10.3 中断改造 → 10.1 裸机
- [ ] **CH4**：`initcall` → `container_of`/侵入式链表 → `.ko` 加载（**整章待补写**）
- [ ] **CH5 / CH6**：素材已就位，按需查阅
- [x] 🗑️ 已删 73 篇硬件内容

完整取舍与补写顺序见 **[00 · 本书取舍与补写顺序](./00-ROADMAP-本书取舍与补写顺序.md)**。
