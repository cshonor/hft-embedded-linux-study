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
cd 00-Linux-Kernel-DPDK-Network-C/04-Kernel-Prep-Embedded-C-Self-Cultivation
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


---

## 章节自测

> 工具链是嵌入式开发的武器。看代码 → 想答案 → 点开验证。

### Q1: gcc 编译四阶段

```bash
# 以下命令分别做什么？
gcc -E main.c -o main.i    # (1)
gcc -S main.c -o main.s    # (2)
gcc -c main.c -o main.o    # (3)
gcc main.o -o app          # (4)

# 如果只写 gcc main.c -o app 呢？
```

<details>
<summary>答案与复习指引</summary>

1. `-E` 预处理：展开 `#include`/`#define` → `.i` 纯 C 源码
2. `-S` 编译：C → 汇编 `.s`
3. `-c` 汇编+编译：C → 目标文件 `.o`（一步到位）
4. 链接：`.o` + 库 → 可执行文件

`gcc main.c -o app` 一步完成全部四阶段。

**复习：** → [1.1 GCC 工具链](./1.1-gcc/1.1-GCC工具链.md)

</details>

### Q2: objdump 反汇编

```bash
# 这条命令做什么？
objdump -dS main.o

# -d 和 -S 分别是什么？
```

<details>
<summary>答案与复习指引</summary>

**答案：**
- `-d` 反汇编（disassemble）`.text` 段
- `-S` 交替显示源码和汇编（需要 `-g` 编译）

**用途：** 读编译器生成的汇编，理解 C 代码到指令的映射。内核调试、性能分析、HFT 热路径优化必备。

**复习：** → [1.4 ELF 二进制分析工具](./1.4-elf-binary-tools/1.4-ELF二进制分析工具.md)

</details>

### Q3: make 基本规则

```makefile
app: main.o utils.o
	gcc -o app main.o utils.o

main.o: main.c utils.h
	gcc -c main.c

utils.o: utils.c utils.h
	gcc -c utils.c
```

> 如果只改了 `utils.c`，执行 `make` 会重新编译哪些文件？为什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 只重新编译 `utils.o` 和重新链接 `app`。`main.o` 不重新编译（依赖 `main.c` 和 `utils.h` 没改）。

`make` 比较目标文件和依赖文件的时间戳，只编译过时的目标。这是增量编译的基础。

**复习：** → [1.3 Makefile](./1.3-makefile/1.3-Makefile.md)
