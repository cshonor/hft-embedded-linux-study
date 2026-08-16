# 第 5 章 内存堆栈管理

**Memory and Stack Management**

## 本章目标

建立 **进程五段 + heap/stack** 与 **Linux VMA** 的完整图景；掌握 **ARM 栈帧**（`push`/`bl`/`ldmfd`、`FP/LR/SP`）与 **GDB `bt`/`x`**；理解 **glibc chunk/碎片/泄漏** 及裸机/RTOS 堆；会用 **mmap** 映射文件与 **MMIO**；熟练 **Valgrind/mtrace/core dump/ASAN** 查内存错误。衔接 **ch04 ELF**、**LKD 虚拟内存**、**DPDK hugepage/mempool**。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../ch01-tools-of-the-trade/)** | `gcc`、`gdb`、`make` |
| **[ch03](../ch03-arm-architecture-and-assembly/)** | AAPCS、`push`/`pop`、`bl`、`objdump -dS` |
| **[ch04](../ch04-compile-link-install-run/)** | ELF `.text/.data/.bss`、`execve`、BSS、动态库 |

## 环境

- **主机**：`gcc -g`、GDB、**binutils**（`size`、`readelf`、`nm`）
- **检测**：`valgrind`、`libc` **mtrace**（`apt install glibc-tools` 或自带）
- **拓展**：`-fsanitize=address`、ARM 交叉 GDB
- **demo/**：见下（勿改 demo 源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch05-memory-stack-management/demo

make all
size demo01_memory_zone demo02_stack_frame demo05_static

# 五段地址
./demo01_memory_zone
readelf -S demo01_memory_zone | grep -E 'text|rodata|data|bss'
cat /proc/self/maps | head

# ARM/x86 栈帧 + GDB
gdb -batch -ex 'break recurse' -ex run -ex bt -ex 'info reg sp fp lr' ./demo02_stack_frame

# 栈溢出（见 demo/README 完整 GDB 步骤）
make demo03_stack_overflow CFLAGS="-g -O0 -Wall -fno-stack-protector"
gdb -batch -ex 'break smash_frame' -ex run -ex 'info reg sp lr' -ex continue -ex bt ./demo03_stack_overflow

# 静态局部 vs 栈局部
./demo05_static

# 泄漏 + Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./demo04_heap_leak

make clean
```

## 六大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 五段与进程** | **5.1**、**5.2** | `.text/.rodata/.data/.bss/heap/stack`；VMA、`/proc/maps`；衔接 **ch04 ELF** |
| **2 栈与调用** | **5.3.*** | 栈初始化；**5.3.2** `stmfd`/`ldmfd`、`bl`/`LR`、栈帧；传参/形实参；**GDB `bt`/`x`** |
| **3 堆管理** | **5.4.*** | 裸机/uC/OS/glibc **chunk**；碎片；**5.4.5** 自研堆；**5.4.1** vs **5.2** MMU |
| **4 作用域与静态** | **5.3.5** | 静态局部 → `.bss/.data`；自动变量 → 栈 |
| **5 溢出与 mmap** | **5.3.6**、**5.5.***、**5.7.*** | canary/ASAN；**mmap** 文件/**MMIO**；共享 `.so` |
| **6 泄漏与检测** | **5.6.***、**5.7.2**、**5.7.5** | 泄漏/mtrace；**core dump**；**Valgrind**；mprotect guard |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | 五段地址打印 | **5.1**、**5.2** |
| **demo02** | 递归栈帧 + GDB | **5.3.2**、**5.3.3** |
| **demo03** | 栈溢出 + GDB（越界/递归） | **5.3.6**、**5.7.2** |
| **demo04** | 故意堆泄漏 | **5.4.3**、**5.6.1**、**5.7.5** |
| **demo05** | 静态局部 vs 局部 | **5.3.5** |
| **demo06** | 工具链合练：`maps`/`mtrace`/core（命令行） | **5.2**、**5.6.3**、**5.7.2** |

**demo06 参考**（无独立源文件）：

```bash
MALLOC_TRACE=/tmp/m.log ./demo04_heap_leak; cat /tmp/m.log | tail
ulimit -c unlimited; ./demo04_heap_leak   # 配合 SIG 实验
pmap -x $$
```

## 考核要点

1. 画出进程 **五段** 并说明与 **ELF 段**、`/proc/maps` 的对应  
2. 写出 ARM 函数调用 **入栈/出栈** 序列；解释 **`bl` 与 LR**  
3. 用 GDB 对 `demo02` 执行 **`bt`、`info registers`、`x/16wx $sp`**  
4. 对比 **静态局部** 与 **栈局部** 的存储与生命周期（**demo05**）  
5. 说明 glibc **chunk**、**brk vs mmap**、**double free** 后果  
6. 口述 **mmap** 映射文件与 **MMIO 设备** 的用户态步骤  
7. 用 **Valgrind** 解读 `definitely lost`；简述 **mtrace** 用法  
8. 配置 **core dump** 并用 GDB **`bt full`** 分析 SIGSEGV  
9. 列举嵌入式 **栈/堆** 三条安全规则（无 MMU 场景）  
10. 对比 **kmalloc** / **DPDK rte_malloc** 与 glibc `malloc` 的使用场景  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 工具、**ch03** ARM 汇编、**ch04** 编译链接 ELF |
| 后置 | **ch06** GNU C 扩展；**ch07** 指针；**ch10** OS/多任务 |

## 小节

- [5.1 程序运行的「马甲」：进程](./5.1-程序运行的-马甲-进程.md)
- [5.2 Linux环境下的内存管理](./5.2-Linux环境下的内存管理.md)
- [5.3 栈的管理](./5.3-stack/5.3-栈的管理.md)
  - [5.3.1 栈的初始化](./5.3-stack/5.3.1-栈的初始化.md)
  - [5.3.2 函数调用](./5.3-stack/5.3.2-函数调用.md)
  - [5.3.3 参数传递](./5.3-stack/5.3.3-参数传递.md)
  - [5.3.4 形参与实参](./5.3-stack/5.3.4-形参与实参.md)
  - [5.3.5 栈与作用域](./5.3-stack/5.3.5-栈与作用域.md)
  - [5.3.6 栈溢出攻击原理](./5.3-stack/5.3.6-栈溢出攻击原理.md)
- [5.4 堆内存管理](./5.4-heap/5.4-堆内存管理.md)
  - [5.4.1 裸机环境下的堆内存管理](./5.4-heap/5.4.1-裸机环境下的堆内存管理.md)
  - [5.4.2 uC/OS的堆内存管理](./5.4-heap/5.4.2-uCOS的堆内存管理.md)
  - [5.4.3 Linux堆内存管理](./5.4-heap/5.4.3-Linux堆内存管理.md)
  - [5.4.4 堆内存测试程序](./5.4-heap/5.4.4-堆内存测试程序.md)
  - [5.4.5 实现自己的堆管理器](./5.4-heap/5.4.5-实现自己的堆管理器.md)
- [5.5 mmap映射区域探秘](./5.5-mmap/5.5-mmap映射区域探秘.md)
  - [5.5.1 将文件映射到内存](./5.5-mmap/5.5.1-将文件映射到内存.md)
  - [5.5.2 mmap映射实现机制分析](./5.5-mmap/5.5.2-mmap映射实现机制分析.md)
  - [5.5.3 把设备映射到内存](./5.5-mmap/5.5.3-把设备映射到内存.md)
  - [5.5.4 多进程共享动态库](./5.5-mmap/5.5.4-多进程共享动态库.md)
- [5.6 内存泄漏与防范](./5.6-memory-leak/5.6-内存泄漏与防范.md)
  - [5.6.1 一个内存泄漏的例子](./5.6-memory-leak/5.6.1-一个内存泄漏的例子.md)
  - [5.6.2 预防内存泄漏](./5.6-memory-leak/5.6.2-预防内存泄漏.md)
  - [5.6.3 内存泄漏检测：MTrace](./5.6-memory-leak/5.6.3-内存泄漏检测-MTrace.md)
  - [5.6.4 广义上的内存泄漏](./5.6-memory-leak/5.6.4-广义上的内存泄漏.md)
- [5.7 常见的内存错误及检测](./5.7-memory-errors/5.7-常见的内存错误及检测.md)
  - [5.7.1 总有一个Bug，让你泪流满面](./5.7-memory-errors/5.7.1-总有一个Bug-让你泪流满面.md)
  - [5.7.2 使用core dump调试段错误](./5.7-memory-errors/5.7.2-使用core-dump调试段错误.md)
  - [5.7.3 什么是内存踩踏](./5.7-memory-errors/5.7.3-什么是内存踩踏.md)
  - [5.7.4 内存踩踏监测：mprotect](./5.7-memory-errors/5.7.4-内存踩踏监测-mprotect.md)
  - [5.7.5 内存检测神器：Valgrind](./5.7-memory-errors/5.7.5-内存检测神器-Valgrind.md)


---

## 代码自测

**题目 1：** 以下代码会导致什么问题？如何用 gdb 定位？
```c
void func(void) {
    char buf[8];
    strcpy(buf, "Hello, World! This is too long.");
}

int main(void) {
    func();
    return 0;
}
```
<details>
<summary>参考答案</summary>

会导致栈溢出（stack overflow / buffer overflow）：buf 只有 8 字节，strcpy 写入了 30+ 字节，覆盖了栈上 buf 之后的数据（包括返回地址）。

可能导致：
1. 程序崩溃（Segmentation fault）——返回地址被覆盖后跳转到非法地址
2. 安全漏洞——攻击者可构造特定字符串覆盖返回地址，执行任意代码

用 gdb 定位：
1. gcc -g -fno-stack-protector func.c -o app（编译时关闭栈保护以便复现）
2. gdb ./app
3. run —— 程序崩溃后 gdb 自动停在崩溃位置
4. bt —— 查看回溯，如果返回地址被破坏，bt 会显示乱码
5. x/20x $sp —— 查看栈内存，发现 buf 区域被字符串覆盖
6. info registers —— 查看 PC/LR 是否指向非法地址

这是 ch05 内存与栈管理的核心知识——理解栈帧结构和缓冲区溢出原理。

</details>

## 代码自测

**题目 1：** 嵌入式 C 自我修养这本书的学习路线是什么？为什么说它是标准 C 到内核的桥梁？

<details>
<summary>参考答案</summary>

路线：标准 C → GNU C 扩展 → 嵌入式系统编程 → 操作系统基础。桥梁作用：内核代码大量使用 __attribute__/typeof/container_of/section/weak 等 GNU 扩展，这些标准 C 教材不讲。本书填补了标准 C 到内核驱动开发之间的知识空白。

</details>
