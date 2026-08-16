# 第 6 章 运动的诗章：运行时数据结构

**Runtime Data Structures** — Peter van der Linden, *Expert C Programming*

## 本章目标

建立 **进程内存五区域** 完整图景：**`.text` / `.rodata` / `.data` / `.bss` / 堆 / 栈** 的布局与增长方向；理解 **OS 载入器**（**mmap、ASLR、页权限、拷贝 `.data`、清零 `.bss`**）与 **C 运行时**（**`_start` / crt0 → `main` → `exit`**）的分工。掌握 **栈帧（活动记录）**：**RA、saved RBP、参数、局部变量** 与 **缓冲区溢出**；区分 **`auto` 局部（栈）** 与 **`static` 局部（`.data/.bss`）** 的生命周期。了解 **线程独立栈、共享堆/全局区**、**setjmp/longjmp** 栈帧失效、**brk/mmap 堆** 与 **泄漏 / use-after-free**。能使用 **size、nm、readelf -l、pmap、valgrind** 验证理论，并衔接 **ch05 ELF 段**、**ch07 内存**、**ch02 static 语义**。

## 五区域内存布局（Linux 典型）

```text
  高地址  ┌─────────────────────────┐
          │         栈 stack         │  ← 向下增长（6.5, 6.9）
          │            ↓             │
          ├─────────────────────────┤
          │      （映射区 / 空洞）    │
          ├─────────────────────────┤
          │         堆 heap          │  ← brk / mmap，malloc（6.9）
          ├─────────────────────────┤
          │   .bss  未初始化全局/静态  │
          ├─────────────────────────┤
          │   .data 已初始化全局/静态  │
          ├─────────────────────────┤
          │   .rodata 只读常量        │
          ├─────────────────────────┤
          │   .text  程序代码         │
  低地址  └─────────────────────────┘
         （地址因 ASLR 每次不同，相对顺序稳定）
```

| 区域 | 典型内容 | ELF / 运行时 |
|------|----------|--------------|
| **`.text`** | 函数机器码 | **PT_LOAD R-X** |
| **`.rodata`** | 字符串、`const` | 只读映射 |
| **`.data`** | `int g = 1;` | 文件 **拷贝** 初值 |
| **`.bss`** | `int g;`、`static` 无初值 | **NOBITS**，载入 **清零** |
| **heap** | `malloc` | **非** ELF 节，crt 后管理 |
| **stack** | `auto` 局部、RA | **非** ELF 节，每线程独立 |

## 载入 vs 运行时（分工）

```text
  ELF (a.out) ──execve──► OS loader (6.3) ──► _start / crt (6.4) ──► main
                mmap          权限/bss           堆/stdio           你的代码
```

| 阶段 | 负责 | 关键动作 |
|------|------|----------|
| **链接 ch05** | **ld** | 合并 **`.text/.data/.bss`** |
| **载入 6.3** | **内核** | **mmap PT_LOAD**、**ASLR**、**bss zero-fill** |
| **CRT 6.4** | **crt + libc** | **堆初始化**、调 **`main`**、**`exit`** |
| **调用 6.5** | **编译器** | **栈帧**、**call/ret** |

## 栈帧与 A→B→C 调用链

```text
  main 栈帧:  main_local, RA→crt
    └─ A:     a_local, RA→main, saved RBP
         └─ B: b_local, RA→A
              └─ C: c_local, RA→B   （demo03：地址递减）
```

**demo04**：越界写 **相邻栈变量**；真实攻击常覆盖 **RA**（**`-fstack-protector`**）。

## auto vs static 局部（生命周期）

| | **`auto` 局部** | **`static` 局部** |
|---|-----------------|-------------------|
| 存储 | **栈帧** | **`.data` / `.bss`** |
| 函数返回后 | 失效（**不可再引用**） | **仍保留** |
| demo02 | `auto_cnt` 每次 **1** | `static_cnt` **1,2,3** |

## 堆：brk / mmap 与常见错误

| 机制 | 说明 |
|------|------|
| **brk/sbrk** | 扩展 **program break** |
| **mmap** | 大块堆、映射文件 |
| **泄漏** | `malloc` 无 **`free`** → RSS 涨（**valgrind**） |
| **wild pointer** | **`free` 后继续解引用** → 未定义（**6.9**, **ch07 7.6**） |

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch05](../ch05-thinking-of-linking/)** | **ELF 段**、**BSS NOBITS**、**四阶段链接**、**readelf / nm** |
| **[Expert C ch02](../ch02-its-not-a-bug-its-a-language-feature/)** | **`static` 内部链接**与语义（**2.4**, **demo04_static_dual**） |
| **[Expert C ch04](../ch04-arrays-are-not-pointers/)** | 存储与 **`sizeof`**（栈上数组 vs 指针） |
| **[01-Primer-K-and-R-C](../../01-Primer-K-and-R-C/)** | 变量、函数、作用域基础 |

## 环境

- **OS**：Linux / WSL（**pmap**、**valgrind**、**readelf**）
- **编译器**：GCC 或 Clang，`gcc --version`
- **推荐 flags**：`-std=c11 -Wall -Wextra -g`
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/03-Advanced-Expert-C-Programming/ch06-runtime-data-structures/demo

# 6.2 五区域地址
cd demo01_memory_layout && make && ./demo01 && cd ..

# 6.6 static vs auto
cd demo02_static_local && make && ./demo02 && nm demo02 | grep static && cd ..

# 6.5 栈帧 A→B→C
cd demo03_stack_frame && make && ./demo03 && cd ..

# 6.5 栈溢出（相邻变量）
cd demo04_buffer_overflow && make && ./demo04 && cd ..

# 6.11 工具
size demo/demo01_memory_layout/demo01
readelf -l demo/demo01_memory_layout/demo01
objdump -x demo/demo01_memory_layout/demo01 | head -30
valgrind --leak-check=full demo/demo01_memory_layout/demo01
pmap -x $$    # 当前 shell 映射
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 可执行文件** | **6.1** | **a.out 传说**；ELF vs 历史 a.out |
| **2 段布局** | **6.2** | **五区域**；**.text/.rodata/.data/.bss**/堆/栈 |
| **3 OS 载入** | **6.3** | **mmap**；**ASLR**；页权限；**bss 清零** |
| **4 C 运行时** | **6.4** | **`_start`**；**crt0**；**非 main 入口** |
| **5 栈帧** | **6.5** | **活动记录**；**RA**；**demo03/demo04** |
| **6 存储类** | **6.6** | **auto vs static** 局部；生命周期表 |
| **7 线程** | **6.7** | **独立栈**；共享 **data/heap** |
| **8 非本地跳转** | **6.8** | **setjmp/longjmp**；栈帧失效 |
| **9 UNIX 堆栈** | **6.9** | **brk/mmap**；泄漏；**野指针** |
| **10 历史** | **6.10** | **MS-DOS** 段寄存器；**near/far** |
| **11 工具** | **6.11** | **size, nm, objdump, readelf, pmap, valgrind** |
| **12 轻松一下** | **6.12** | **CMU** 编程谜题 |
| **13 进阶** | **6.13** | **信号栈**；**alloca**；**VLA**；**内核栈** |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_memory_layout** | 打印 **text/rodata/data/bss/heap/stack** 地址 | **6.2**, **6.9**, **6.11** |
| **demo02_static_local** | **`auto_cnt` vs `static_cnt`** 三次调用 | **6.6** |
| **demo03_stack_frame** | **main→A→B→C** 栈地址递减 | **6.5**, **6.7** |
| **demo04_buffer_overflow** | 越界覆盖 **相邻 `auth`** | **6.5**, **6.3** NX/canary |

## 高频考点 / 面试题

1. **`.data` 和 `.bss` 区别？载入时如何处理？** → **`.data`** 有初值、从 ELF **拷贝**；**`.bss`** **NOBITS**、内存 **零填充**（**6.2**, **6.3**, **ch05 5.1**）

2. **程序入口是 `main` 吗？** → **否**；**`_start`（crt）** 初始化后调 **`main`**，返回走 **`exit`**（**6.4**）

3. **函数内 `static int x` 和 `int x` 存储在哪？** → **`static`** → **`.data/.bss`**，生命周期整个进程；**`auto`** → **栈帧**，返回失效（**6.6**, **demo02**）

4. **栈帧里通常有什么？缓冲区溢出为何危险？** → **参数、RA、saved RBP、局部变量**；覆盖 **RA** 可劫持控制流（**6.5**, **demo04**）

5. **多线程共享什么？** → **`.text/.rodata/.data/.bss`/堆** 共享需同步；**每线程独立栈**（**6.7**）

**拓展：**

- **堆如何增长？`brk` 与 `mmap` 分工？**（**6.9**）
- **`setjmp/longjmp` 后 auto 局部为何不可信？**（**6.8**）
- **ASLR 影响哪些区域？**（**6.3**）
- **`valgrind` 报 use-after-free 对应哪类 bug？**（**6.9**, **6.11**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[Expert C ch05](../ch05-thinking-of-linking/)** ELF 段与链接；**ch02** **static**；**ch04** 存储 |
| 后置 | **[ch07](../ch07-adventures-in-memory/)** 虚拟内存、Cache、泄漏、总线错误 |
| 关联 | **[Embedded C](../../05-Kernel-Prep-Embedded-C-Self-Cultivation/)** 启动代码、链接脚本、裸机栈 |
| 全书 | **ch08** 类型；**ch09** 数组再论 |

## 小节

- [6.1 a.out及其传说](./6.1-a-out及其传说.md)
- [6.2 段](./6.2-段.md)
- [6.3 操作系统在a.out文件里干了些什么](6.3-操作系统在a-out文件里干了些什么.md)
- [6.4 C语言运行时系统在a.out里干了些什么](6.4-C语言运行时系统在a-out里干了些什么.md)
- [6.5 当函数被调用时发生了什么：过程活动记录](./6.5-当函数被调用时发生了什么-过程活动记录.md)
- [6.6 auto和static关键字](./6.6-auto和static关键字.md)
- [6.7 控制线程](./6.7-控制线程.md)
- [6.8 setjmp和longjmp](./6.8-setjmp和longjmp.md)
- [6.9 UNIX中的堆栈段](./6.9-UNIX中的堆栈段.md)
- [6.10 MS-DOS中的堆栈段](./6.10-MS-DOS中的堆栈段.md)
- [6.11 有用的C语言工具](./6.11-有用的C语言工具.md)
- [6.12 轻松一下——卡耐基-梅隆大学的编程难题](./6.12-轻松一下卡耐基-梅隆大学的编程难题.md)
- [6.13 只适用于高级学员阅读的材料](./6.13-只适用于高级学员阅读的材料.md)


---

## 章节自测

> 运行时数据结构 = 程序在内存中的真实布局。看代码 → 想答案 → 点开验证。

### Q1: 五段内存布局

```c
int g_init = 42;         // (1)
int g_zero;               // (2)
const char *g_str = "hello"; // (3) "hello" 在哪段？
void foo(void) {}         // (4) foo 代码在哪段？
int main(void) {
    int local = 7;         // (5)
    int *heap = malloc(4); // (6)
    return 0;
}
```

> 六个变量各在进程的哪个段？

<details>
<summary>答案与复习指引</summary>

| 变量 | 段 | 原因 |
|------|-----|------|
| `g_init = 42` | `.data` | 已初始化全局变量 |
| `g_zero` | `.bss` | 未初始化全局变量（自动清零） |
| `"hello"` 字面量 | `.rodata` | 字符串常量只读 |
| `g_str` 指针 | `.data` | 已初始化全局指针 |
| `foo` 代码 | `.text` | 函数代码 |
| `local` | 栈 | 局部变量 |
| `heap` 指向的内存 | 堆 | `malloc` 分配 |

**复习：** → [6.1 a.out 不是无中生有](6.1-a-out及其传说.md)

</details>

### Q2: 栈帧结构

```c
// 调用 foo(3, 4) 时的栈帧（x86-64 简化）
void foo(int a, int b) {
    int local = a + b;
    // ...
}
```

> 栈帧里有哪些东西？返回地址存在哪？

<details>
<summary>答案与复习指引</summary>

**栈帧内容（从高到低）：**
1. **参数** `a=3, b=4`（x86-64 前几个参数用寄存器，多余的压栈）
2. **返回地址**（`call` 指令自动压入，函数返回时 `ret` 弹出）
3. **保存的 RBP**（上一帧的基址指针）
4. **局部变量** `local`

**安全风险：** 如果 `local` 是 `char buf[8]`，用户写入超过 8 字节 → 覆盖返回地址 → **缓冲区溢出攻击**（经典 stack smashing）。

**复习：** → [6.4 栈帧](6.4-C语言运行时系统在a-out里干了些什么.md) · [6.5 缓冲区溢出](6.5-当函数被调用时发生了什么-过程活动记录.md)

</details>

### Q3: setjmp/longjmp 栈帧失效

```c
jmp_buf env;

void func_a(void) {
    if (setjmp(env) == 0) {
        printf("first call\n");
        func_b();
    } else {
        printf("jumped back\n");
    }
}

void func_b(void) {
    int big_array[1000];
    big_array[0] = 42;
    longjmp(env, 1);  // 跳回 setjmp
    // big_array 还在吗？
}
```

> `longjmp` 跳回后 `func_b` 的 `big_array` 还有效吗？为什么不能返回栈局部？

<details>
<summary>答案与复习指引</summary>

**答案：** `longjmp` 回滚栈指针到 `setjmp` 时的位置。`func_b` 的栈帧被丢弃，`big_array` 不再有效。后续函数调用会复用那块栈内存。

**关键：** `longjmp` 不能跳回**已返回的函数**的栈帧——那块内存已被复用，跳过去读到的是垃圾数据。

**用途：** 异常处理（C 没有异常机制）、协程、深层递归快速退出。

**复习：** → [6.9 UNIX中的堆栈段](./6.9-UNIX中的堆栈段.md)
