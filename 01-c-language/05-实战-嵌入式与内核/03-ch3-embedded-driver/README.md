# CH3 · 嵌入式驱动中的 GNU C 实战

**Embedded Drivers: GNU C in Action**

> 一句话概括这一章：**同样是 C，为什么写寄存器时要写成这样。**
> CH1/CH2 讲工具本身，这一章讲「跟硬件打交道时，哪把工具必须上」。

> **前置**：[CH1 基础扩展语法](../01-ch1-gnu-c-basics/) · [CH2 高级特性](../02-ch2-gnu-c-advanced/)
> **取舍依据**：[00 · 本书取舍与补写顺序](../00-ROADMAP-本书取舍与补写顺序.md)

## 本章讲什么

> **同样是 X，为什么在写嵌入式/驱动代码时要写成这样，而不是普通 C 那样？**
> 判据只有一条：

> **同样是 X，为什么在写嵌入式/驱动代码时要写成这样，而不是普通 C 那样？**

判据只有一条：

> **CSAPP 讲硬件不讲 C，标准 C 教材讲 C 不讲硬件——中间这段才是嵌入式 C 真正的地盘。**

所以本章**一条硬件原理都不写**。Cache 一致性、字节序成因、MMU 页表、流水线冒险——这些交给 CSAPP。这里只写**它们在 C 里长什么样、写错了编译器会不会告诉你、以及怎么写才是对的**。

## 内容归属说明

本章是从原书不同章节里**抽出来的嵌入式相关小节**，不是某一章的连续内容：

| 本目录 | 原书位置 | 为什么归到这里 |
|--------|---------|--------------|
| [10.8 嵌入式 C 开门](./10.8-register/10.8-寄存器操作.md) | 原 10.8 寄存器操作 | 已改写成 C 语言视角（volatile / 屏障 / 位操作 / 字节序 / 未对齐） |
| [10.1 裸机多任务](./10.1-bare-metal/) | 原 10.1 | 裸机环境是嵌入式 C 的默认背景 |
| [10.3 中断](./10.3-interrupt/) | 原 10.3 | ISR 与主循环的共享数据保护，纯 C/并发问题 |
| ~~原 4.14 链接脚本~~ | 已移到 [CH6](../06-ch6-toolchain-custom/4.14-链接脚本.md) | section 属性 + 链接脚本 = 把代码钉到物理地址 |
| ~~原 3.7 GNU ARM 工具链~~ | 已移到 [CH6](../06-ch6-toolchain-custom/3.7-gnu-arm) | 交叉编译、objdump -dS 读反汇编 |

> 本章只留**纯 C 表达层面**的三节：寄存器怎么读写、ISR 怎么共享数据、没有 OS 时怎么切上下文。
> 「把代码放到哪里去」交给 [CH6](../06-ch6-toolchain-custom/)；进程/线程/文件系统等 OS 通识留在 [附录 A](../90-ref-os/)。

## 学习路线（驱动实战三步走）

| 序 | 小节 | 状态 | 关键结论 |
|----|------|------|---------|
| 1 | [10.8 嵌入式 C 开门](./10.8-register/10.8-寄存器操作.md) | ✅ 29 KB | `-O2` 下不加 volatile 的反汇编**只剩 `jmp <自己>`**；volatile 不原子（20 万次 ×2 只得到 276660） |
| 2 | [4.14 链接脚本](../06-ch6-toolchain-custom/4.14-链接脚本.md) | 待扩 | `section` 属性如何把变量钉到指定段 |
| 3 | [10.3 中断](./10.3-interrupt/) | 待改造 | ISR 与主循环的共享数据：`volatile` + `__atomic_*` |
| 4 | [10.1 裸机多任务](./10.1-bare-metal/) | 待扩 | 没有 OS 时怎么切上下文 |
| 5 | [3.7 GNU ARM 工具链](../06-ch6-toolchain-custom/3.7-gnu-arm) | 待扩 | 交叉编译 + `objdump -dS` 读反汇编 |

> 建议按 1 → 2 → 3 的顺序：先学会「跟硬件打交道时 C 该怎么写」，再看链接脚本把代码放哪，最后才是中断这种并发场景。

## CH1 / CH2 的工具，在 CH3 怎么用

CH1、CH2 讲**工具本身**，这一章讲**什么时候必须用它**：

| CH1 / CH2 的扩展 | CH3 的使用场景 |
|-------------|----------------|
| `volatile` 属性 | 寄存器轮询、ISR 共享标志（[10.8](./10.8-register/10.8-寄存器操作.md)） |
| `__attribute__((packed))` | 硬件寄存器映射结构体、网络报文头（[10.8](./10.8-register/10.8-寄存器操作.md)） |
| `__attribute__((aligned(n)))` | DMA 缓冲区、cache line 对齐（[10.8](./10.8-register/10.8-寄存器操作.md)） |
| `__attribute__((weak))` | 板级 hook、可选驱动（[6.9](../02-ch2-gnu-c-advanced/6.9-weak/6.9-属性声明-weak.md)） |
| `__attribute__((section(".x")))` | 把数据/代码钉到指定地址（[4.14](../06-ch6-toolchain-custom/4.14-链接脚本.md)） |
| `__builtin_bswap` / `ffs` / `popcount` | 字节序转换、位扫描（[10.8](./10.8-register/10.8-寄存器操作.md)） |
| 柔性数组 | 变长报文、DMA 描述符（[6.5](../01-ch1-gnu-c-basics/6.5-zero-length-array/6.5-零长度数组.md)） |

## 三条铁律（10.8 实测得来）

1. **`volatile` 管不了原子性，也管不了 Cache。** 它只约束编译器：每次都真的去读内存。多线程/ISR 下计数必须换 `__atomic_*`。
2. **`packed` 会让你绕过 UBSan。** 编译器知道对齐就是 1，生成的是合法访问，没有 UB 可报。真雷是**取地址往外传**（`-Waddress-of-packed-member` 是唯一防线）。
3. **`aligned` 管不了 `malloc`。** 类型上的 `aligned(64)` 对堆块无效，要 `aligned_alloc`。

---

## 代码自测

<details>
<summary><strong>题目 1：</strong>这个目录为什么不讲 Cache 一致性、MMU、字节序成因？</summary>

因为那些是**硬件知识**，CSAPP 讲得更深也更系统，重复读第二遍没有收益。

这里的判据是：**CSAPP 讲硬件不讲 C，标准 C 教材讲 C 不讲硬件——中间这段才是嵌入式 C 的地盘。**

所以本目录只写它们在 C 里的**表达形式**：
- Cache → `volatile` **管不了**它；cache line 64 B 决定 `aligned(64)` 和 padding
- 字节序 → `__builtin_bswap*` + 主机序探测
- MMU/地址译码 → 设备寄存器在 C 里的三种表达（`volatile` 指针、`packed` 结构体、`BIT/GENMASK` 掩码）

</details>

<details>
<summary><strong>题目 2（判断）：</strong>给 `uint32_t` 计数器加了 `volatile`，ISR 和主循环就能安全地共享它。</summary>

**不够。**

`volatile` 只保证「每次都真的读内存、不缓存到寄存器」，它**不保证 RMW 的原子性**。实测两个线程各 20 万次 `++`，`volatile` 版只得到 **276660 / 400000**。

正确做法：
- 单写者（只有 ISR 写、主循环读）且数据是机器字长 → `volatile` 够用（`sig_atomic_t` 就是这个场景）
- 多写者、或需要 RMW → `__atomic_add_fetch` / `__atomic_load_n`（或关中断）

另外注意 `volatile` **也管不了 Cache**：它不产生任何 Cache 维护指令，DMA 场景必须显式做 Cache 维护。

</details>

<details>
<summary><strong>题目 3：</strong>为什么 `-O2` 下不用 `volatile` 的轮询循环会死循环？</summary>

因为编译器把寄存器的值缓存到了寄存器里。实测反汇编：

```
不加 volatile，-O2：
   jmp <自己>          <- 连读都不读，直接死循环

加 volatile，-O2：
   mov 0x...(%rip),%eax
   test %eax,%eax
   jne ...
```

真跑一个线程置位：非 volatile 版**卡死超时**，volatile 版 0.308 s 正常退出。

这也是为什么「_debug 能跑、-O2 就挂」是嵌入式最经典的 bug 形态。

</details>
