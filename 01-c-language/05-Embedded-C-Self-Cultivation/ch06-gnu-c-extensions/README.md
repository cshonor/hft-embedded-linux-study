# 第 6 章 GNU C 编译器扩展语法精讲

**GNU C Extensions for Kernel, Bootloader & DPDK**

## 本章目标

分清 **ISO C99/C11** 与 **GNU 扩展**；掌握 **`__attribute__`**（packed/aligned/section/weak/format/noinline 等）、**语句表达式**、**typeof/container_of**、**零长度数组**、**指定初始化**、**inline/builtins**、**可变参宏**。能读 **Linux 内核 / U-Boot / DPDK** 头文件宏，并编写带 **format 检查** 的日志与 **weak hook** 默认实现。衔接 **ch05 堆布局**、**ch07 指针**、**ch08 位操作**。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../ch01-tools-of-the-trade/)** | `gcc`、`make`、基本命令行 |
| **[ch04](../ch04-compile-link-install-run/)** | 编译链接、ELF 段、符号表、`nm`/`readelf` |
| **[ch05](../ch05-memory-stack-management/)** | 堆 `malloc`、结构体布局、对齐概念 |

## 环境

- **编译器**：GCC 或 Clang，**`-std=gnu11`**（与内核一致）
- **工具**：`gcc -E`（预处理）、`objdump`、`readelf`、`nm`
- **拓展**：内核头或 **demo/**（由课程提供，见下）
- **可选**：`-Wall -Wformat=2 -Wpedantic` 对比标准/扩展差异

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/demo

make all

# packed 布局与 sizeof
./demo01_packed_struct
gdb -batch -ex run -ex 'x/16xb &p' ./demo01_packed_struct

# section 符号与段
readelf -S demo02_custom_section | grep my_
nm demo02_custom_section | grep g_custom

# weak 默认 vs 强符号覆盖
./demo03_weak
./demo03_weak_override

# constructor 先于 main
./demo04_constructor

# 零长度数组 / FAM
./demo05_flexible_array

# 日志宏 + 语句表达式 MAX
./demo06_log_macro

# 内嵌汇编读计数器（x86_64 / AArch64）
./demo07_reg_asm

make clean
```

## 六大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 标准与扩展** | **6.1** | C89–C23 演进；`-std=`；GNU 扩展地图 |
| **2 数据初始化** | **6.2**、**6.5** | 指定初始化；零长度数组 / FAM |
| **3 宏与类型** | **6.3**、**6.4**、**6.12** | 语句表达式；typeof；container_of；`##__VA_ARGS__` |
| **4 链接与布局** | **6.6**、**6.7**、**6.9** | section；aligned/packed；weak/alias |
| **5 函数与诊断** | **6.8**、**6.10** | format 属性；inline/noinline |
| **6 编译器内建** | **6.11** | `__builtin_constant_p`；expect；likely/unlikely |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | `packed` 结构体布局 | **6.7.4**、**6.7.2** |
| **demo02** | `section` 自定义段 | **6.6.2**、**6.6.3** |
| **demo03** | `weak` 默认与板级覆盖 | **6.9.1–6.9.3** |
| **demo04** | `constructor` 初始化顺序 | **6.6.1** |
| **demo05** | 零长度数组动态分配 | **6.5.2** |
| **demo06** | 日志宏 + `format` | **6.8.3**、**6.12** |
| **demo07** | 内嵌汇编 / 寄存器约束 | **6.11.2**、ch03 汇编 |

## 考核要点

1. 解释 **`-std=c11`** 与 **`-std=gnu11`** 对语句表达式、零长度数组的影响  
2. 写出 **`container_of` 指针运算** 并说明 `offsetof` 作用  
3. 对比 **FAM** 与 **指针成员** 的分配与序列化场景（**6.5.4**）  
4. 用 **`readelf`/`nm`** 定位 **section 属性** 放置的符号（**demo02**）  
5. 说明 **`packed` 与 `aligned`** 同时使用时可能的对齐陷阱  
6. 实现带 **`format(printf,m,n)`** 的两层日志宏（固定参 + `...`）  
7. 描述 **weak 符号** 链接规则及与强符号冲突行为（**demo03**）  
8. 对比 **inline 函数** 与 **语句表达式宏** 的优缺点  
9. 解释 **`likely`/`unlikely`** 与 **`__builtin_expect`** 的作用  
10. 写出 **`LOG(fmt, ...)`** 使用 **`##__VA_ARGS__`** 的原因，并列举内核/DPDK 实例  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch04** 编译链接 ELF；**ch05** 内存对齐与堆 |
| 后置 | **ch07** 指针；**ch08** 位操作；**ch09** 预处理；**ch10** OS |

## 小节

- [6.1 C语言标准和编译器](./6.1-c-standard/6.1-C语言标准和编译器.md)
  - [6.1.1 什么是C语言标准](./6.1-c-standard/6.1.1-什么是C语言标准.md)
  - [6.1.2 C语言标准的内容](./6.1-c-standard/6.1.2-C语言标准的内容.md)
  - [6.1.3 C语言标准的发展过程](./6.1-c-standard/6.1.3-C语言标准的发展过程.md)
  - [6.1.4 编译器对C语言标准的支持](./6.1-c-standard/6.1.4-编译器对C语言标准的支持.md)
  - [6.1.5 编译器对C语言标准的扩展](./6.1-c-standard/6.1.5-编译器对C语言标准的扩展.md)
- [6.2 指定初始化](./6.2-designated-init/6.2-指定初始化.md)
  - [6.2.1 指定初始化数组元素](./6.2-designated-init/6.2.1-指定初始化数组元素.md)
  - [6.2.2 指定初始化结构体成员](./6.2-designated-init/6.2.2-指定初始化结构体成员.md)
  - [6.2.3 Linux内核驱动注册](./6.2-designated-init/6.2.3-Linux内核驱动注册.md)
  - [6.2.4 指定初始化的好处](./6.2-designated-init/6.2.4-指定初始化的好处.md)
- [6.3 宏构造「利器」：语句表达式](./6.3-statement-expr/6.3-宏构造-利器-语句表达式.md)
  - [6.3.1 表达式、语句和代码块](./6.3-statement-expr/6.3.1-表达式-语句和代码块.md)
  - [6.3.2 语句表达式](./6.3-statement-expr/6.3.2-语句表达式.md)
  - [6.3.3 在宏定义中使用语句表达式](./6.3-statement-expr/6.3.3-在宏定义中使用语句表达式.md)
  - [6.3.4 内核中的语句表达式](./6.3-statement-expr/6.3.4-内核中的语句表达式.md)
- [6.4 typeof与container_of宏](./6.4-typeof-container-of/6.4-typeof与container_of宏.md)
  - [6.4.1 typeof关键字](./6.4-typeof-container-of/6.4.1-typeof关键字.md)
  - [6.4.2 typeof使用示例](./6.4-typeof-container-of/6.4.2-typeof使用示例.md)
  - [6.4.3 Linux内核中的container_of宏](./6.4-typeof-container-of/6.4.3-Linux内核中的container_of宏.md)
  - [6.4.4 container_of宏实现分析](./6.4-typeof-container-of/6.4.4-container_of宏实现分析.md)
- [6.5 零长度数组](./6.5-zero-length-array/6.5-零长度数组.md)
  - [6.5.1 什么是零长度数组](./6.5-zero-length-array/6.5.1-什么是零长度数组.md)
  - [6.5.2 零长度数组使用示例](./6.5-zero-length-array/6.5.2-零长度数组使用示例.md)
  - [6.5.3 内核中的零长度数组](./6.5-zero-length-array/6.5.3-内核中的零长度数组.md)
  - [6.5.4 思考：指针与零长度数组](./6.5-zero-length-array/6.5.4-思考-指针与零长度数组.md)
- [6.6 属性声明：section](./6.6-section/6.6-属性声明-section.md)
  - [6.6.1 GNU C编译器扩展关键字：__attribute__](./6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md)
  - [6.6.2 属性声明：section](./6.6-section/6.6.2-属性声明-section.md)
  - [6.6.3 U-boot镜像自复制分析](./6.6-section/6.6.3-U-boot镜像自复制分析.md)
- [6.7 属性声明：aligned](./6.7-aligned/6.7-属性声明-aligned.md)
  - [6.7.1 地址对齐：aligned](./6.7-aligned/6.7.1-地址对齐-aligned.md)
  - [6.7.2 结构体的对齐](./6.7-aligned/6.7.2-结构体的对齐.md)
  - [6.7.3 思考：编译器一定会按照aligned指定的方式对齐吗](./6.7-aligned/6.7.3-思考-编译器一定会按照aligned指定的方式对齐吗.md)
  - [6.7.4 属性声明：packed](./6.7-aligned/6.7.4-属性声明-packed.md)
  - [6.7.5 内核中的aligned、packed声明](./6.7-aligned/6.7.5-内核中的aligned-packed声明.md)
- [6.8 属性声明：format](./6.8-format/6.8-属性声明-format.md)
  - [6.8.1 变参函数的格式检查](./6.8-format/6.8.1-变参函数的格式检查.md)
  - [6.8.2 变参函数的设计与实现](./6.8-format/6.8.2-变参函数的设计与实现.md)
  - [6.8.3 实现自己的日志打印函数](./6.8-format/6.8.3-实现自己的日志打印函数.md)
- [6.9 属性声明：weak](./6.9-weak/6.9-属性声明-weak.md)
  - [6.9.1 强符号和弱符号](./6.9-weak/6.9.1-强符号和弱符号.md)
  - [6.9.2 函数的强符号与弱符号](./6.9-weak/6.9.2-函数的强符号与弱符号.md)
  - [6.9.3 弱符号的用途](./6.9-weak/6.9.3-弱符号的用途.md)
  - [6.9.4 属性声明：alias](./6.9-weak/6.9.4-属性声明-alias.md)
- [6.10 内联函数](./6.10-inline/6.10-内联函数.md)
  - [6.10.1 属性声明：noinline](./6.10-inline/6.10.1-属性声明-noinline.md)
  - [6.10.2 什么是内联函数](./6.10-inline/6.10.2-什么是内联函数.md)
  - [6.10.3 内联函数与宏](./6.10-inline/6.10.3-内联函数与宏.md)
  - [6.10.4 编译器对内联函数的处理](./6.10-inline/6.10.4-编译器对内联函数的处理.md)
  - [6.10.5 思考：内联函数为什么定义在头文件中](./6.10-inline/6.10.5-思考-内联函数为什么定义在头文件中.md)
- [6.11 内建函数](./6.11-builtin/6.11-内建函数.md)
  - [6.11.1 什么是内建函数](./6.11-builtin/6.11.1-什么是内建函数.md)
  - [6.11.2 常用的内建函数](./6.11-builtin/6.11.2-常用的内建函数.md)
  - [6.11.3 C标准库的内建函数](./6.11-builtin/6.11.3-C标准库的内建函数.md)
  - [6.11.4 内建函数：__builtin_constant_p(n)](./6.11-builtin/6.11.4-内建函数-__builtin_constant_p-n.md)
  - [6.11.5 内建函数：__builtin_expect(exp,c)](./6.11-builtin/6.11.5-内建函数-__builtin_expect-exp-c.md)
  - [6.11.6 Linux内核中的likely和unlikely](./6.11-builtin/6.11.6-Linux内核中的likely和unlikely.md)
- [6.12 可变参数宏](./6.12-vararg-macro/6.12-可变参数宏.md)
  - [6.12.1 什么是可变参数宏](./6.12-vararg-macro/6.12.1-什么是可变参数宏.md)
  - [6.12.2 继续改进我们的宏](./6.12-vararg-macro/6.12.2-继续改进我们的宏.md)
  - [6.12.3 可变参数宏的另一种写法](./6.12-vararg-macro/6.12.3-可变参数宏的另一种写法.md)
  - [6.12.4 内核中的可变参数宏](./6.12-vararg-macro/6.12.4-内核中的可变参数宏.md)
