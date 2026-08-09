# 第 18 章 运行时环境

**Runtime Environment**

## 本章讲什么

全书**底层收官**：虚拟地址空间、栈帧 ABI、**brk/mmap** 堆、crt0 启动、静/动态链接、缓存行/大页、信号与 gdb/valgrind。打通 **C 代码 ↔ OS ↔ CPU**。

## 学习重点

- 六段：**text/rodata/data/bss/heap/stack**  
- 栈↓ 堆↑；虚拟地址、缺页、MAP_SHARED  
- **x86-64 栈帧**与寄存器传参；禁递归/大栈数组/返回栈指针  
- **crt0 → main**；exit vs abort  
- **-static** vs 动态 .so；static/extern/hidden  
- **brk vs mmap**；DPDK 大页  
- **64B 缓存行**、伪共享、大页 TLB  
- **SIGSEGV/SIGABRT/OOM**；全局初始化顺序  

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | mmap 大页、静态链接、cache line padding |
| 内核 | 用户态 VMA ↔ 内核页表；Oops 栈 |
| HFT | inline 减栈帧、伪共享排查、OOM 定位 |

## 线上陷阱（汇总）

1. 大局部数组栈溢出  
2. 返回栈指针悬垂  
3. 伪共享延迟抖动  
4. .so 版本不匹配  
5. 跨文件全局初始化顺序  
6. malloc 碎片 OOM  
7. 深递归  
8. 写 rodata  

## 实操（建议完成）

1. readelf -S 看各段  
2. gdb 看 rbp/rsp/局部地址  
3. 大数组 SIGSEGV  
4. static vs dynamic 体积/启动  
5. 跨文件全局初始化  
6. 伪共享 padding 对比  
7. valgrind 泄漏/double free  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch02/ch07/ch11/ch14/ch17 |
| 后序 | **全书终章**；K&R UNIX IO |
| 配套 | 《C陷阱与缺陷》ch03/ch04/ch07 |

## 全书闭环

ch01–ch17 语法、内存、IO、ADT → 本章映射到 **ELF、VMA、ABI、syscall**。

## 小节

- [18.1 判断运行时环境](./18.1-determining-runtime-environment/18.1-determining-runtime-environment.md)
- [18.2 C 和汇编语言的接口](./18.2-C和汇编语言的接口.md)
- [18.3 运行时效率](./18.3-运行时效率.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 虚拟地址空间六段

```c
int global_init = 42;    // (1) 哪段？
int global_zero;          // (2) 哪段？
const char *str = "hi";   // (3) "hi" 在哪段？
int main(void) {          // (4) main 代码在哪段？
    int local = 1;        // (5) 哪段？
    int *heap = malloc(4);// (6) 哪段？
}
```

<details>
<summary>答案与复习指引</summary>

**答案：**
1. `.data`（已初始化全局变量）
2. `.bss`（未初始化全局变量，自动清零）
3. `.rodata`（字符串字面量，只读）
4. `.text`（代码段）
5. 栈（`stack`，局部变量）
6. 堆（`heap`，`malloc` 分配）

**布局：** `.text` / `.rodata` / `.data` / `.bss` 在低地址（固定），堆向上增长，栈从高地址向下增长。

**复习：** → [18.1 Determining Runtime](./18.1-determining-runtime-environment/18.1-determining-runtime-environment.md)

</details>

### Q2: 缓存行与伪共享

```c
// 两个线程各操作一个计数器
struct {
    int counter_a;  // 线程 1 写
    int counter_b;  // 线程 2 写
} stats;

// stats 的大小是多少？有什么性能问题？
```

<details>
<summary>答案与复习指引</summary>

**答案：** `sizeof = 8`（两个 `int`）。两个计数器在同一**缓存行**（64 字节）内 → **伪共享（false sharing）**。线程 1 写 `counter_a` 使整条缓存行失效 → 线程 2 的 `counter_b` 也要重新从内存加载。

**HFT 性能杀手：** 伪共享可使多线程性能下降 10 倍以上。

**修复：** 用 `__attribute__((aligned(64)))` 或 padding 让每个计数器独占一个缓存行。

```c
struct {
    int counter_a;
    char pad[60];   // 填到 64 字节
    int counter_b;
} stats;
```

**复习：** → [18.3 运行时效率](./18.3-运行时效率.md)
