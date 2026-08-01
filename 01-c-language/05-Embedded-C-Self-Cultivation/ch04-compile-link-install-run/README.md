# 第 4 章 程序的编译、链接、安装和运行

**Compile, Link, Install and Run**

## 本章目标

建立 **从 `.c` 到进程运行** 的完整链路：GCC 四阶段（`-E/-S/-c`/link）、ELF 段与符号/重定位、静/动态库（`ar`/PIC/GOT/PLT）、安装与 `exec` 装载、`_start`→`main` 与 BSS；并能用 **readelf/nm/objdump** 排查链接错误。拓展 **链接脚本**（裸机 Flash/RAM）、内核模块与 U-Boot 重定位，衔接 LKD 与 DPDK 构建。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../ch01-tools-of-the-trade/)** | `gcc`、`make`、`gdb`、`objdump` |
| **[ch02](../ch02-computer-architecture-and-cpu/)** | 内存、端序、ISA |
| **[ch03](../ch03-arm-architecture-and-assembly/)** | 汇编、`.section`、AAPCS、`objdump -dS` |

## 环境

- **主机**：`gcc`、`binutils`（`readelf`、`nm`、`ar`、`objdump`、`ld`）
- **交叉（拓展）**：`arm-none-eabi-gcc` + 链接脚本（**4.14**）
- **demo/**：四阶段、静/动库示例（见下，勿改 demo 源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch04-compile-link-install-run/demo

# 四阶段
make demo01
readelf -S demo01.o demo01
nm demo01.o

# 静库 / 动库
make demo_static demo_shared
./demo_static
./demo_shared
nm libdemo.a
ldd demo_shared

# 工具
readelf -h demo01
objdump -h demo01
make clean
```

## 八大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 四阶段** | 4.1–4.3 | `-E/-S/-c`/link；`.i/.s/.o` |
| **2 ELF 与段** | 4.1、4.3.3、4.13 | `.o/.a/.so`/可执行；`.text/.data/.bss` |
| **3 链接三步** | 4.4 | 分段组装、符号决议、重定位 |
| **4 静/动库** | 4.7、4.8 | `ar`、PIC、GOT、PLT、`.so` |
| **5 链接脚本** | **4.14** | `MEMORY`/`SECTIONS`；Flash/RAM |
| **6 安装与运行** | 4.5、4.6 | 部署、`exec`、`_start`、`main`、BSS |
| **7 常见错误** | 4.4.2、4.7 | undefined reference、multiple definition |
| **8 书外拓展** | 4.9–4.12 | 插件、内核模块、内核/U-Boot 启动 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | 四阶段 `demo01.o` / 可执行 | **4.1**、**4.3** |
| **demo_static** | 静库 `libdemo.a` | **4.7** |
| **demo_shared** | 共享库 + `ldd` | **4.8** |
| **工具练习** | `readelf`/`nm`/`objdump` | **4.13** |

## 考核要点

1. 写出 GCC **四阶段** 与对应选项；说明 `.o` 与可执行 ELF 的 `Type` 差异  
2. 用 `readelf -S` / `nm` 解释 **`.text/.data/.bss`** 与 `U/T/D/B` 符号  
3. 口述链接 **三步**；举例 `undefined reference` 与 **静库顺序** 修复  
4. 对比 **静库 `ar`** 与 **动态库 `-fPIC`**；说明 GOT/PLT 与延迟绑定  
5. 画 Linux 下 **`execve` → ld.so → _start → main`** 链路  
6. 说明 **BSS** 为何不占文件体积、谁清零  
7. 读一段最小 **链接脚本**，指出 Flash 与 RAM 段  
8. 列举 **binutils** 五项常用命令及用途  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 工具链、**ch02** 体系结构、**ch03** 汇编 |
| 后置 | **ch05** 堆栈与内存布局；**ch06** GNU C；**ch09** 模块化；**ch10** OS |

## 小节

- [4.1 从源程序到二进制文件](./4.1-从源程序到二进制文件.md)
- [4.2 预处理过程](./4.2-预处理过程.md)
- [4.3 程序的编译](./4.3-compilation/4.3-程序的编译.md)
  - [4.3.1 从C文件到汇编文件](./4.3-compilation/4.3.1-从C文件到汇编文件.md)
  - [4.3.2 汇编过程](./4.3-compilation/4.3.2-汇编过程.md)
  - [4.3.3 符号表与重定位表](./4.3-compilation/4.3.3-符号表与重定位表.md)
- [4.4 链接过程](./4.4-linking/4.4-链接过程.md)
  - [4.4.1 分段组装](./4.4-linking/4.4.1-分段组装.md)
  - [4.4.2 符号决议](./4.4-linking/4.4.2-符号决议.md)
  - [4.4.3 重定位](./4.4-linking/4.4.3-重定位.md)
- [4.5 程序的安装](./4.5-installation/4.5-程序的安装.md)
  - [4.5.1 程序安装的本质](./4.5-installation/4.5.1-程序安装的本质.md)
  - [4.5.2 在Linux下制作软件安装包](./4.5-installation/4.5.2-在Linux下制作软件安装包.md)
  - [4.5.3 使用apt-get在线安装软件](./4.5-installation/4.5.3-使用apt-get在线安装软件.md)
  - [4.5.4 在Windows下制作软件安装包](./4.5-installation/4.5.4-在Windows下制作软件安装包.md)
- [4.6 程序的运行](./4.6-execution/4.6-程序的运行.md)
  - [4.6.1 操作系统环境下的程序运行](./4.6-execution/4.6.1-操作系统环境下的程序运行.md)
  - [4.6.2 裸机环境下的程序运行](./4.6-execution/4.6.2-裸机环境下的程序运行.md)
  - [4.6.3 程序入口main()函数分析](./4.6-execution/4.6.3-程序入口main-函数分析.md)
  - [4.6.4 BSS段的小秘密](./4.6-execution/4.6.4-BSS段的小秘密.md)
- [4.7 链接静态库](./4.7-链接静态库.md)
- [4.8 动态链接](./4.8-dynamic-linking/4.8-动态链接.md)
  - [4.8.1 与地址无关的代码](./4.8-dynamic-linking/4.8.1-与地址无关的代码.md)
  - [4.8.2 全局偏移表](./4.8-dynamic-linking/4.8.2-全局偏移表.md)
  - [4.8.3 延迟绑定](./4.8-dynamic-linking/4.8.3-延迟绑定.md)
  - [4.8.4 共享库](./4.8-dynamic-linking/4.8.4-共享库.md)
- [4.9 插件的工作原理](./4.9-插件的工作原理.md)
- [4.10 Linux内核模块运行机制](./4.10-Linux内核模块运行机制.md)
- [4.11 Linux内核编译和启动分析](./4.11-Linux内核编译和启动分析.md)
- [4.12 U-boot重定位分析](./4.12-U-boot重定位分析.md)
- [4.13 常用的binutils工具集](./4.13-常用的binutils工具集.md)
- [4.14 链接脚本](./4.14-链接脚本.md)
