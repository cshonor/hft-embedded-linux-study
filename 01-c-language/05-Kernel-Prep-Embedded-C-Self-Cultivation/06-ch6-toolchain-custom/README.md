# CH6 · GNU C 编译链在嵌入式与内核中的定制

**Customizing the Toolchain for Embedded & Kernel Work**

> 一句话概括这一章：**代码写完之后，是谁把它变成在目标机上跑的东西。**
> 前五章都在写 C 源代码；这一章决定它的**地址、符号可见性、加载方式**。

> **前置**：[CH2](../02-ch2-gnu-c-advanced/) 的 `section` / `weak`——这一章是它们在工具链侧的兑现
> **配套**：utility 环境见 [1 工具链](./1-toolchain/)

## 本章讲什么

嵌入式和内核场景有个共同点：**没有默认的东西**。

- 没有操作系统帮你加载你 → 你得告诉链接器**代码放哪个物理地址**（链接脚本）
- 没有标准启动流程 → 你得决定 `_start` 是谁、栈指针初值是多少
- 不能直接编译 → 得用**交叉工具链**（在 x86 上编出 ARM 的机器码）
- 符号不能全都可见 → 得控制哪些导出、哪些静态化

前五章的答案都在源码里；这一章的答案在 **.map / .ld / 编译选项** 里。

## 章节导航

| 序 | 目录 / 文件 | 原书位置 | 核心问题 |
|----|------------|---------|---------|
| 1 | [1 工具链](./1-toolchain/) | 原第 1 章 | 编辑器 / make / git / **ELF 分析工具** / gdb |
| 2 | [2 编译与链接](./2-compile-and-link/) | 原第 4 章 | 四阶段、静态库、动态库、装载与运行 |
| 3 | [3.7 GNU ARM 汇编与伪操作](./3.7-gnu-arm/) | 原 3.7 | **交叉编译**、`.section` 伪操作、`objdump -dS` 读反汇编 |
| 4 | [4.14 链接脚本](./4.14-链接脚本.md) | 原 4.14 | **把代码钉到指定地址**——本章最硬的一节 |

## 四块怎么串起来

```text
1 工具链          -> 你手上有什么：gcc / binutils / gdb / 交叉编译器
     |
2 编译与链接      -> 默认流程是什么：预处理->编译->汇编->链接->装载
     |                + 静态库(.a) 与动态库(.so) 的本质差别
     |                + GOT / PLT / PIC（见 2/4.8-dynamic-linking）
     v
4.14 链接脚本     -> 默认流程不够用：我需要代码从 0x87800000 开始跑
     |                + PROVIDE / ENTRY / MEMORY / >region AT>region
     |                + __image_copy_start 这类符号怎么来
     v
3.7 GNU ARM      -> 换个目标就有了新约束：交叉工具链 + 汇编伪操作
                     + objdump -dS 验证「编译器到底优化成什么样了」
```

> **建议顺序**：2（搞懂默认） → 4.14（知道怎么改） → 3.7（换到交叉环境） → 1（缺什么补什么）。

## 与前五章的接口

| 前五章学到的 | 在这一章兑现为 |
|-------------|--------------|
| [CH2 `section`](../02-ch2-gnu-c-advanced/6.6-section/) | 链接脚本把该段放到指定地址；`.initcallN.init` 收集成表（→ [CH4](../04-ch4-kernel-module/)） |
| [CH2 `weak`](../02-ch2-gnu-c-advanced/6.9-weak/6.9-属性声明-weak.md) | `nm` 输出里的 `W`/`V`；强弱决议规则；**为什么不触发静态库提取** |
| [CH2 `packed`/`aligned`](../02-ch2-gnu-c-advanced/6.7-aligned/6.7-属性声明-aligned.md) | `-S` 看结构体字段偏移；`-Waddress-of-packed-member` 警告 |
| [CH1 `inline`](../01-ch1-gnu-c-basics/6.10-inline/) | `-O2`/`-flto` 下内联是否真的发生；`objdump -dS` 是唯一裁判 |
| [CH5 符号可见性](../05-ch5-portable-modular/) | `-fvisibility=hidden`、`EXPORT_SYMBOL`、静态库 vs 动态库的符号表 |
| [CH3 volatile](../03-ch3-embedded-driver/10.8-register/10.8-寄存器操作.md) | `-O2` 下挥发性访问是否被合并；只能看反汇编 |

**一句话**：前五章的每一条「静默出错」，都要靠这一章的工具查出来。

## 必会工具清单

| 命令 | 用途 | 在本书哪里用得多 |
|------|------|----------------|
| `nm` | 看符号类型（`T/D/B/W/V/C/U`） | CH2 6.9 weak 全篇 |
| `readelf -S` | 看段表（有哪些 section、地址、大小） | 4.14 链接脚本 |
| `objdump -dS` | 源码+汇编对照，**验证优化是否发生** | CH1 6.10 inline / CH3 10.8 volatile |
| `ld --verbose` | 打印默认链接脚本 | 4.14（写自定义脚本的模板） |
| `gcc -S` / `-E` | 看编译/预处理结果 | 2/4.3-compilation |
| `ar t` / `ar x` | 看静态库成员（**理解"成员提取"的前提**） | CH2 6.9 + 2/4.7 |
| `size` | 各段大小（嵌入式上斤斤计较） | 4.14 |
| `file` | 目标文件架构（交叉编译第一件事） | 3.7 |

## 衔接

- **应用层**（进程、线程、内存管理）不在本章：见 [附录 A OS 通识](../90-ref-os/) · [附录 B 内存堆栈](../91-ref-memory/)
- **构建产物怎么排错**：见 [2/4.4 链接过程](./2-compile-and-link/4.4-linking/)
- **内核模块怎么被加载**（动态链接的内核版）：见 [CH4 4.10](../04-ch4-kernel-module/4.10-Linux内核模块运行机制.md)

<details><summary>代码自测（点击展开）</summary>

**Q1：链接脚本和 `-Wl,-Ttext=0x8000` 有什么区别？**

<details><summary>答案</summary>
`-Ttext` 只是把 `.text` 的起始地址改了，其余段仍在默认布局里；链接脚本是**完全接管**——你能指定每个段的地址、对齐、加载地址(LMA)与运行地址(VMA) 分离（`AT>`），还能 `PROVIDE` 自定义符号。U-Boot 重定位就是靠 **LMA ≠ VMA**：先从 Flash(LMA) 加载进去，代码自己把自己搬到 RAM(VMA) 跑。详见 [4.14](./4.14-链接脚本.md) 与 [CH4 4.12 U-Boot 重定位](../04-ch4-kernel-module/4.12-U-boot重定位分析.md)。
</details>

**Q2：为什么要交叉编译？在目标机上装个 gcc 不行吗？**

<details><summary>答案</summary>
多数嵌入式目标机资源不足以跑编译器（几 MB 内存、无文件系统），而且没有标准库和目标头文件。**交叉编译器**在 x86 主机上跑，产出目标架构的机器码，配套的 sysroot 提供目标平台的头文件与库。所以命令名带了前缀——`arm-none-eabi-gcc` 里 `none` 表示无操作系统、`eabi` 表示嵌入式 ABI。详见 [3.7](./3.7-gnu-arm/)。
</details>

**Q3：静态库和一堆 `.o` 直接链接有什么本质区别？**

<details><summary>答案</summary>
静态库是 `.o` 的打包，**但链接器只会提取当前未解析符号需要的成员**。这意味着：库里有强符号覆盖你的 weak 定义，但如果那个成员恰好没被提取，覆盖就不生效——这正是 [CH2 6.9](../02-ch2-gnu-c-advanced/6.9-weak/6.9-属性声明-weak.md) 实测的头号坑（六组对照）。用 `-Wl,--whole-archive` 或干脆直接链 `.o` 可绕开。
</details>

</details>
