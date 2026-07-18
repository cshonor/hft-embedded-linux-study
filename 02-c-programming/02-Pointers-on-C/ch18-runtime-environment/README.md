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
