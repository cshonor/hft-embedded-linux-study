# CH2 · GNU C 高级特性

**GNU C Extensions — Compiler & Linker Level**

> 一句话概括这一章：**源码里看不见，但真实发生的扩展**。
> CH1 的语法你看得懂就完事了；**这一章的每一条都能让代码通过编译、通过链接，然后在某个夜里崩掉**。

所以这一章的笔记全部带 **WSL 实测**（gcc 13.3 / clang 18.1.3 / GNU ld 2.42），不靠记忆推断。

## 为什么必须单独成一章

CH1 的东西写错会**编译报错**，你立刻知道。CH2 的东西写错：

| 陷阱 | 表面现象 | 真实原因 |
|------|---------|---------|
| `weak` + 静态库 | hook 静默不生效，`addr=(nil)` | [6.9](./6.9-weak/6.9-属性声明-weak.md)：weak **不触发静态库成员提取** |
| `packed` 取地址 | ARM 上 Bus Error | [6.7](./6.7-aligned/6.7-属性声明-aligned.md)：未对齐访问的 ABI 后果 |
| `aligned` 用在 malloc 上 | 地址照样不对齐 | `aligned` 管不了运行时分配器 |
| `if (hook)` 判空 | `-O2` 下判空被优化掉 | 编译器证明了 weak 符号地址非 NULL |
| 忘记 `volatile` | 循环被优化成死循环 | [CH3 10.8](../03-ch3-embedded-driver/10.8-register/10.8-寄存器操作.md) |

**共同点：编译器按你写的做，而不是按你想的做。**

## 章节导航

| 序 | 小节 | 状态 | 作用在哪个阶段 |
|----|------|------|--------------|
| 1 | [6.6 `__attribute__` 总纲](./6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) | ✅ 37 KB | 语法 + 全属性表（**先读这篇当字典**） |
| 2 | [6.7 aligned 与 packed](./6.7-aligned/6.7-属性声明-aligned.md) | ✅ 32 KB | **ABI / 内存布局** |
| 3 | [6.8 format（格式串检查）](./6.8-format/6.8-属性声明-format.md) | ✅ 30 KB | 编译期诊断（archetype 支持矩阵 / `-Wformat` 家族 / `no_printk` 零开销） |
| 4 | [6.9 weak 与 alias](./6.9-weak/6.9-属性声明-weak.md) | ✅ 31 KB | **链接期**（唯一一条） |
| 5 | [6.11 内建函数（likely/unlikely/popcount/overflow）](./6.11-builtin/6.11-内建函数.md) | ✅ 40 KB | 编译期优化 + 分支布局 + 位操作 + 溢出检查 |
| 6 | [3.6 内联汇编（四段式/约束/clobber/volatile/asm goto）](./3.6-mixed-programming/3.6-C语言和汇编语言混合编程.md) | ✅ 27 KB | **代码生成**（操作数约束矩阵 / earlyclobber / 漏 clobber 的 gcc-clang 分裂 / volatile 删除实测 / asm goto） |
| 附 | [demo/](./demo/) | 8 个示例 + [08-format](./demo/08-format/) 12 个 + [09-builtin](./demo/09-builtin/) 14 个 + [10-asm](./demo/10-asm/) 15 个 | 本章全部可编译复现 |

## 三条主线一看就懂的分工

```text
        编译期                    链接期                   运行期
   ------------------      -------------------      ------------------
   6.6  __attribute__      6.9  weak / alias        6.7  packed 的取值代价
   6.7  aligned/packed          （强弱决议）          6.11 likely/unlikely
   6.8  format 检查                                  3.6  内联汇编的副作用
   6.11 __builtin_expect                             volatile（见 CH3）
```

**只有 `weak` 一条是纯粹的链接期语义**，其余都在「编译期决定编码/布局」。

## 阅读顺序

```text
6.6  __attribute__ 总纲     <- 先知道属性这套语法长什么样，后面全是它的实例
  |
6.7  aligned / packed       <- 布局控制。这是最容易"看着对、跑着崩"的一条
  |
6.9  weak / alias           <- 唯一链接期的。读这篇之前要能看懂 nm 输出的 T/D/B/V/W
  |
6.11 likely / unlikely      <- 短，但每次读热点路径代码都会遇到
  |
6.8  format                 <- 写自己的 log 库时再看
  |
3.6  内联汇编               <- 独立话题，需要时能看懂 __asm__ 的冒号分段即可
```

## 与 CH1 的呼应：`inline` vs `weak`

这两个是**语义对立**的一对，务必对照着读：

| | [CH1 6.10 `inline`](../01-ch1-gnu-c-basics/6.10-inline/) | [CH2 6.9 `weak`](./6.9-weak/6.9-属性声明-weak.md) |
|---|---|---|
| 对「谁能看见这个符号」 | 尽量**摊平到调用点**，希望消失 | 保留符号，**允许被别人替换** |
| 编译器能不能内联 | 能 | **永远不能**（实测热路径慢 **3.2 倍**） |
| 多个定义怎么办 | 每个 TU 一份（`static inline`） | 决议：强胜 / 两弱取第一个 |
| 典型场景 | 热路径小函数 | 冷路径可选钩子、测试替身 |

> 实测数据：2 亿次调用，weak 0.131 s vs strong(内联) 0.041 s。反汇编里 weak 的循环体是 `call <addw>`，strong 是 `lea 0x1(%rax)` 内联展开。
> **结论：热路径不要用 `weak`。**

## 衔接

- **前置**：[CH1 · GNU C 基础扩展语法](../01-ch1-gnu-c-basics/)（`typeof` / 语句表达式是读懂 `__attribute__` 用法的前提）
- **出口一**：[CH3 嵌入式驱动实战](../03-ch3-embedded-driver/)——`packed` 的位域、`volatile`、内存屏障
- **出口二**：[CH4 内核模块应用](../04-ch4-kernel-module/)——`section`/`weak`/`alias` 在驱动注册里的角色
- **工具**：反汇编与 nm 的用法见 [CH6 2-compile-and-link](../06-ch6-toolchain-custom/2-compile-and-link/)

<details><summary>代码自测（点击展开）</summary>

**Q1：`__attribute__((packed))` 之后取成员地址，为什么在某些平台上会崩？**

<details><summary>答案</summary>
`packed` 取消了填充字节，成员可能落在未对齐地址。x86 容忍未对齐访问（只是慢），但部分 ARM 配置下会触发对齐异常（Bus Error/alignment fault）。而且一旦对该成员取地址传给 `uint32_t*`，ARM 编译器会假定指针自然对齐——见 [6.7](./6.7-aligned/6.7-属性声明-aligned.md) 实测。
</details>

**Q2：为什么 `if (weak_func)` 判空有时会被优化掉？**

<details><summary>答案</summary>
如果本翻译单元里 weak 函数**有定义**，编译器能证明它的地址必然非 NULL，`-O2` 下就直接删掉 `test/je`。只有「本 TU 里仅有 weak 声明」时才保留判空。推论：想做可选钩子，默认实现不能出现在调用点所在 TU——详情见 [6.9](./6.9-weak/6.9-属性声明-weak.md)。
</details>

**Q3：`likely(x)` 和手写 `if (x)` 差别到底多大？**

<details><summary>答案</summary>
它不改变逻辑，只给编译器一个分支概率提示，用来重排代码布局（把热分支放在 fall-through 路径上）并改善指令 Cache / 预取。收益取决于分支是否真的可预测——放错了会更慢。详见 [6.11.6](./6.11-builtin/6.11.6-Linux内核中的likely和unlikely.md)。
</details>

</details>
