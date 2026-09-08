# Ch12 · The C memory model（C 内存模型） ②🔴

> **Level 2 · 相知** · 策略：**🔴 精读** · 阅读顺序 ②
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

> **第 4 本书 · Ch12** · 本章是全书对 HFT 最关键的两章之一（另一章是 Ch21 原子）。

## 本章讲什么

C 的统一内存模型：所有对象本质上都是字节数组。**Effective Type** 规则决定"能否把一块内存
当某类型访问"——这是 DPDK 零拷贝解析网络报文的理论基础。对齐（alignment）决定数据在内存中
的摆放规则——`alignas(64)` 防伪共享是 HFT 核心技巧。

## 小节索引

| 节 | 标题 | 核心知识点 |
|----|------|------------|
| [12.1](./12.1-统一内存模型.md) | 统一内存模型 | 对象 = 字节序列；`unsigned char *` 可别名任何类型 |
| [12.2](./12.2-union.md) | union | 类型双关的三种合法方式；C11 允许读取非活跃成员 |
| [12.3](./12.3-内存和状态.md) | 内存和状态 | `volatile` 语义；volatile vs `_Atomic` vs 内存屏障 |
| [12.4](./12.4-指向非指定对象的指针.md) | 指向非指定对象的指针 | `void *` 通用指针；不能解引用/做指针算术 |
| [12.5](./12.5-显式转换.md) | 显式转换 | 整数 widening/narrowing；指针合法/非法转换 |
| [12.6](./12.6-有效类型.md) | 有效类型 | **核心**：DPDK 零拷贝理论基础；何时可以 cast |
| [12.7](./12.7-对齐.md) | 对齐 | `alignas(64)` 防伪共享；结构体布局优化 |

## HFT / DPDK 关联总结

| 概念 | HFT 应用 |
|------|----------|
| **Effective type** | DPDK 零拷贝报文解析（malloc 内存 cast 成协议头） |
| **严格别名** | 不要用指针强转做类型双关，用 `memcpy` 或 `union` |
| **`alignas(64)`** | 缓存行对齐防伪共享（rte_ring head/tail 分离） |
| **`char *` 别名** | 安全地以字节视角访问任何数据 |
| **`volatile`** | MMIO 寄存器、信号 flag（不做多线程同步！） |
| **`_Atomic`** | 多线程共享数据（详见 Ch21） |
| **结构体布局** | 按大小降序排列减少 padding |

## 自测题

<details><summary>1. 为什么能安全地把 malloc 返回的内存 cast 成任意结构体指针？</summary>

malloc 返回的内存没有 effective type（它是原始字节）。通过任意类型的左值写入时，
effective type 变为该左值的类型。所以可以安全地 cast 成 `struct eth_hdr *` 并访问字段。
但注意：如果内存来自声明变量 `struct foo x;`，则 effective type 已固定为 `struct foo`，
不能再 cast 成其它不兼容类型。DPDK 零拷贝解析正是利用 malloc 内存无 effective type 的特性。
</details>

<details><summary>2. <code>*(uint32_t *)&amp;f</code>（f 是 float）为什么是 UB？正确做法是什么？</summary>

float 变量的 effective type 是 `float`，通过 `uint32_t *` 访问违反严格别名规则（strict aliasing）。
编译器开启 `-O2` 后可能产生错误结果。正确做法：
① `memcpy(&bits, &f, sizeof(bits))`——编译器优化为零成本；
② `union { float f; uint32_t u; }`——C11 明确允许读取非活跃成员。
</details>

<details><summary>3. 伪共享是什么？怎么用 <code>alignas</code> 解决？</summary>

伪共享：多个 CPU 核频繁修改同一缓存行（64 字节）中的不同变量，导致缓存行在核间反复失效和传输，
性能大幅下降。解决：用 `alignas(64)` 把需要独立修改的变量放到不同缓存行。
DPDK rte_ring 的 head/tail 各自 `alignas(64)` 就是为了让生产者和消费者操作不同缓存行。
</details>

<details><summary>4. <code>volatile</code> 能做多线程同步吗？为什么？</summary>

不能。`volatile` 只保证每次访问都真正读写内存（不缓存到寄存器），但不保证：
① 原子性（多字节读写可能被中断撕裂）；② 内存序（不阻止编译器/CPU 重排非 volatile 访问）；
③ 可见性（不保证其它核心看到最新值）。多线程同步必须用 C11 `_Atomic`（带内存序）或锁。
内核中用 `smp_wmb()`/`smp_rmb()` 等内存屏障。
</details>

<details><summary>5. 什么时候用 <code>unsigned char *</code> 访问其它类型的数据？</summary>

当你需要以字节视角查看对象底层表示时：① 检查字节序；② 实现序列化/反序列化；
③ 实现 `memcpy`/`memcmp` 等字节级操作；④ 检查内存内容（调试）。
`unsigned char *` 是唯一可以合法别名任何类型的指针（C 标准保证）。
</details>
