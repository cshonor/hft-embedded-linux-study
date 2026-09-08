# 《嵌入式 C 语言自我修养》

**从芯片、编译器到操作系统** · 王利涛

> **第 5 本书** · GNU C 扩展（标准 → 内核的桥）
> **主线**：GNU C 扩展 · 嵌入式 C 落地

## 怎么读这本书（2026-09-08 重组）

这本书**不按原书章节顺序读**。硬件/体系结构别的书讲得更深（CSAPP），重复读没意义——本书只取两块：

> **GNU C 扩展**（`01-gnu-c-extensions/`，本书唯一核心）+ **嵌入式 C 落地**（`02-embedded-c/`）

🗑️ 原第 2 章（计算机体系结构，48 篇）、原 3.1–3.5 / 3.9（ARM 指令，25 篇）**共 73 篇已物理删除，文件夹也一并删掉**。
恢复命令：`git log --diff-filter=D --name-only -- 01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/`

---

## 主线一 · GNU C 扩展 —— [01-gnu-c-extensions/](./01-gnu-c-extensions/)

**本书唯一核心。** 标准 C 教材不讲、但你读 Linux 内核 / U-Boot / DPDK 源码时每行都会撞见的东西。

| 序 | 小节 | 状态 |
|----|------|------|
| 1 | [6.3 语句表达式](./01-gnu-c-extensions/6.3-statement-expr/6.3-宏构造-利器-语句表达式.md) | ✅ 29 KB |
| 2 | [6.4 typeof 与 container_of](./01-gnu-c-extensions/6.4-typeof-container-of/6.4-typeof与container_of宏.md) | ✅ 28 KB |
| 3 | [6.5 零长度数组与柔性数组](./01-gnu-c-extensions/6.5-zero-length-array/6.5-零长度数组.md) | ✅ 39 KB |
| 4 | [6.6 `__attribute__` 总纲](./01-gnu-c-extensions/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) | ✅ 37 KB |
| 5 | [6.7 aligned 与 packed](./01-gnu-c-extensions/6.7-aligned/6.7-属性声明-aligned.md) | ✅ 32 KB |
| 6 | [6.9 weak 与 alias](./01-gnu-c-extensions/6.9-weak/6.9-属性声明-weak.md) | ✅ 31 KB |
| 7 | [6.10 inline](./01-gnu-c-extensions/6.10-inline/) | ▶ 下一批 |
| 8 | 6.11 内建函数 · 6.12 变参宏 · 6.2 指定初始化 · 6.1 C 标准 | 待扩 |
| 附 | [3.6 内联汇编](./01-gnu-c-extensions/3.6-mixed-programming/)（`__asm__ __volatile__` 本质是 GNU 扩展） | 待扩 |

## 主线二 · 嵌入式 C 落地 —— [02-embedded-c/](./02-embedded-c/)

**把 GNU C 扩展用起来。** 不写硬件原理，只写「这件事怎么用 C 表达」。

| 序 | 小节 | 状态 |
|----|------|------|
| 1 | [10.8 嵌入式 C 开门](./02-embedded-c/10.8-register/10.8-寄存器操作.md)（volatile / 屏障 / 位操作 / 字节序 / 未对齐） | ✅ 29 KB |
| 2 | [10.1 裸机多任务](./02-embedded-c/10.1-bare-metal/) | 待扩 |
| 3 | [10.3 中断](./02-embedded-c/10.3-interrupt/)（ISR 与主循环共享数据） | 待改造 |
| 4 | [4.14 链接脚本](./02-embedded-c/4.14-链接脚本.md) | 待扩 |
| 5 | [3.7 GNU ARM 工具链](./02-embedded-c/3.7-gnu-arm/)（交叉编译、`objdump -dS`） | 待扩 |

## 支撑与参考（按需查阅）

| 目录 | 原章节 | 内容 | 什么时候看 |
|------|--------|------|-----------|
| [03-toolchain](./03-toolchain/) | ch01 | vim / make / git / ELF 工具 / gdb | 环境搭不起来时 |
| [04-compile-and-link](./04-compile-and-link/) | ch04 | 编译链接、静态库、动态链接、内核模块 | 主线二写链接脚本前 |
| [05-memory-and-stack](./05-memory-and-stack/) | ch05 | 内存堆栈管理 | 栈溢出排查时 |
| [06-pointers-and-data](./06-pointers-and-data/) | ch07 | 数据存储与指针 | 主线一读 container_of 前 |
| [07-oop-in-c](./07-oop-in-c/) | ch08 | C 的面向对象 | 有余力时 |
| [08-modular-c](./08-modular-c/) | ch09 | 模块化编程 | 有余力时 |
| [09-os-reference](./09-os-reference/) | ch10 剩余 | 进程/线程/文件系统/IO/MMU | OS 通识，别处更深 |
| [10-arm-asm-reference](./10-arm-asm-reference/) | ch03 剩余 | AArch64 选读 | 极少用 |

## 定位

Linux 内核、内核模块、DPDK 代码大量依赖 GNU-C 扩展；标准 C 教材不讲这些内容。读 LKD、虚拟内存、内核网络源码前需先过主线一。
主线二则解决「知道了扩展怎么用，但不知道在什么场合该用」。

## 学习进度

- [x] 主线一：6.3 / 6.4 / 6.5 / 6.6 / 6.7 / 6.9 已精写（约 196 KB，全部 WSL 实测）
- [ ] 主线一：6.10 inline → 6.11 builtin → 6.12 变参宏 → 6.2 / 6.1
- [x] 主线二：10.8 嵌入式 C 开门（29 KB）
- [ ] 主线二：10.3 中断 → 4.14 链接脚本 → 10.1 裸机 → 3.7 工具链
- [x] 🗑️ 已删 73 篇硬件内容（原 ch02 整章、ch03.1–3.5/3.9）

完整取舍与补写顺序见 **[00 · 本书取舍与补写顺序](./00-ROADMAP-本书取舍与补写顺序.md)**。

---

## 代码自测

**题目 1：** 嵌入式 C 自我修养这本书的核心定位是什么？它填补了标准 C 和内核开发之间的什么知识空白？
```c
// 以下内核代码用到了哪些本书讲解的知识点？
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}
```
<details>
<summary>参考答案</summary>

这段内核代码用到了本书讲解的多个知识点：
1. container_of 宏（ch06 GNU C 扩展）：使用 typeof、语句表达式、零指针技巧，从链表节点指针获取宿主结构体指针
2. 侵入式链表（ch08 OOP in C）：list_head 结构体嵌入到其他结构体中，实现通用链表
3. static inline（ch09 模块化编程）：头文件中的内联函数，避免函数调用开销
4. 指针运算（ch07 数据存储与指针）：container_of 内部用 char* 指针减法计算偏移

本书的核心定位：标准 C 到内核开发的桥梁。内核代码大量使用 GNU C 扩展（__attribute__/typeof/section/weak）、OOP 模式（函数指针模拟虚函数）、模块化设计（头文件/源文件分离），这些标准 C 教材不讲，本书系统讲解。

</details>
