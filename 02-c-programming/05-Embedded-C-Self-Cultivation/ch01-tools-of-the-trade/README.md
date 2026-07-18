# 第 1 章 工欲善其事，必先利其器

**Tools of the Trade**

## 本章目标

掌握嵌入式底层**全套命令行工具链**，脱离 IDE 完成编写、编译、反汇编、调试、构建与版本管理。

## 前置依赖

无 —— 教程入门章。

## 环境要求

**Linux / WSL2**；预装 `gcc gdb make cmake git vim binutils`。

## 快速启动

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation
source devenv/buildenv.sh
# 进入各 demo 目录后
make all && gdb ./app
make clean
```

## 学习重点

| 模块 | 内容 |
|------|------|
| Shell & Vim | 文件/文本/进程；vimrc |
| GCC | **-E/-S/-c** 四阶段；**-Wall -g -O0** |
| ELF 工具 | objdump、readelf、nm、size |
| GDB | break/step/print/x/bt、core |
| Make/CMake | Makefile 自动变量；cmake build/ |
| buildenv | **source** 环境脚本 |
| Git | commit/branch、.gitignore |

## 标准 Demo 清单

1. **Demo01** Shell+Vim + gcc 四阶段  
2. **Demo02** objdump/readelf/nm 分析 ELF  
3. **Demo03** gdb 单步 + 看栈/内存  
4. **Demo04** 多文件 Makefile  
5. **Demo05** source **buildenv.sh** + 交叉编译  
6. **Demo06** Git 提交与 ignore  

## 课后考核

1. 纯命令行完成 C 源码→运行全流程  
2. **gdb + core** 定位崩溃行  
3. 独立编写/使用 **buildenv.sh**  
4. **objdump -d** 读懂基础汇编  
5. 可复用多文件 Makefile（build/clean）  

## 前后章节

| 方向 | 章节 |
|------|------|
| 后置 | ch02–ch10 全程依赖本章命令 |
| 关联 | ch04 编译链接；ch05 core；ch03 汇编 |

## 小节

- [1.1 代码编辑工具：Vim](./1.1-vim/1.1-代码编辑工具-Vim.md)
- [1.2 程序编译工具：make](./1.2-make/1.2-程序编译工具-make.md)
- [1.3 代码管理工具：Git](./1.3-git/1.3-代码管理工具-Git.md)
- [1.4 ELF 二进制分析工具](./1.4-elf-binary-tools/1.4-ELF二进制分析工具.md)
- [1.5 GDB 源码级调试](./1.5-gdb/1.5-GDB源码级调试.md)
- [1.6 环境脚本 buildenv.sh](./1.6-buildenv/1.6-环境脚本-buildenv.md)
