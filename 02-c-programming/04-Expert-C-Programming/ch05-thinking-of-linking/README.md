# 第 5 章 对链接的思考

**Thinking of Linking** — Peter van der Linden, *Expert C Programming*

## 本章目标

建立 **编译链接载入** 完整图景：**cpp → cc1 → as → ld** 四阶段；ELF **`.text` / `.data` / `.bss`（NOBITS 零初始化）**；**符号解析 + 重定位**；**强 / 弱 / static** 符号规则。掌握 **静态库 `.a`**（**`ar rcs`**、从左到右扫描、**`-lm` 顺序**）与 **动态链接**（**PIC**、共享库、**lazy binding**、**ldd**）。理解 **Interpositioning**（**LD_PRELOAD**、**--wrap**）与 **链接器报告**（**`-Wl,-Map=`**、**readelf -r**、**objdump -R**）。能联系 **ch04** 说明 **链接不做 C 类型检查**；了解 **kernel.lds** 中 **ENTRY / _stext / _ebss** 边界符号。

## 四阶段编译流水线

```text
  .c  ──cpp──►  .i  ──cc1──►  .s  ──as──►  .o  ──ld──►  a.out / kernel.elf
       预处理      编译       汇编      链接
```

| 阶段 | 命令示例 | 产物 |
|------|----------|------|
| 预处理 | `gcc -E main.c -o main.i` | 宏展开后的源码 |
| 编译 | `gcc -S main.i -o main.s` | 汇编 |
| 汇编 | `gcc -c main.s -o main.o` | 可重定位目标文件 |
| 链接 | `gcc main.o -o main` 或 `ld -T kernel.lds ...` | 可执行 / 共享库 |

**demo01_four_stage** 分步演示；**demo04_linker_script** 用 **`ld -T kernel.lds`** 裸链 **kernel.elf**。

## 段布局与 BSS

| 段 | 内容 | 示例 |
|----|------|------|
| **`.text`** | 机器码 | 函数体 |
| **`.rodata`** | 只读常量 | 字符串字面量 |
| **`.data`** | 已初始化全局/静态 | `int g = 1;` |
| **`.bss`** | 未初始化全局/静态 | `int g;` → 载入时 **清零**，ELF 常标 **NOBITS** |

```bash
readelf -S main.o
readelf -S main.o | grep -i bss
```

## 符号与链接规则（速查）

| 类型 | 规则 |
|------|------|
| **强符号** | 有初值全局、函数；同名 **不能** 两个强定义 |
| **弱符号** | `int x;` tentative；与强同名 → 用 **强**（**demo03_weak_strong**） |
| **`static`** | 文件内链接，不参与跨文件解析 |
| **`.a` 归档** | 仅 **未定义符号** 触发提取成员（**5.3**） |
| **扫描顺序** | 命令行 **从左到右**；**`-lfoo` 放引用它的 `.o` 之后** |

## 静态库 vs 动态库

| | 静态 `.a` | 动态 `.so` |
|---|-----------|------------|
| 链接 | `ar rcs` + `-lfoo` | `-shared -fPIC` + `-lfoo` |
| 代码副本 | 每程序一份（**5.3 秘密 5**） | 多进程共享 |
| 插桩 | `--wrap` | **LD_PRELOAD** |
| 查看依赖 | `nm -A libfoo.a` | **ldd** |

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch04](../ch04-arrays-are-not-pointers/)** | **`extern char arr[]` ≠ `extern char *arr`**；链接不查类型（**4.2–4.4**） |
| **[Expert C ch03](../ch03-analyzing-c-declarations/)** | 声明读法、**extern** 与定义 |
| **[01-K-and-R-C](../../01-K-and-R-C/)** | 函数、作用域、多文件编译基础 |

## 环境

- **OS**：Linux / WSL（**demo04** 使用 **`ld -T`**，Windows 原生需交叉工具链）
- **编译器**：GCC 或 Clang，`gcc --version`
- **推荐 flags**：`-std=c11 -Wall -Wextra -g`
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch05-thinking-of-linking/demo

# 5.1 四阶段 + inspect
cd demo01_four_stage && make all && make inspect && cd ..

# 5.1 / 5.3 静态库
cd demo02_static_lib && make && ./demo_static && cd ..

# 5.1 强 / 弱符号
cd demo03_weak_strong && make && ./demo_weak && cd ..

# 5.1 / 5.5 链接脚本
cd demo04_linker_script && make && readelf -S kernel.elf && cd ..

# 常用只读命令（任意 .o / a.out）
nm main.o
readelf -S main.o
readelf -r main.o
ar rcs libfoo.a foo.o
ar t libfoo.a
gcc main.o -Wl,-Map=link.map -o main
objdump -R ./main    # 动态可执行
ldd ./main
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 链接基础** | **5.1** | 四阶段；段；符号解析；重定位；nm / readelf |
| **2 动态链接** | **5.2** | PIC；`.so`；lazy binding；ldd |
| **3 静态库秘密** | **5.3** | 左→右扫描；按需拉 `.o`；循环依赖；`-lm`；重复代码 |
| **4 插桩** | **5.4** | LD_PRELOAD；--wrap；malloc 调试 |
| **5 链接报告** | **5.5** | -Wl,-Map=；readelf -r；objdump -R |
| **6 轻松一下** | **5.6** | 图灵测验；幽默复盘 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_four_stage** | `.i/.s/.o` 分步；`g_init`/`g_bss`；`make inspect` | **5.1**, **5.5** |
| **demo02_static_lib** | **`ar rcs libadd.a`**；`-ladd` 顺序 | **5.1**, **5.3** |
| **demo03_weak_strong** | 弱 + 强 → 强定义胜出 | **5.1** |
| **demo04_linker_script** | **kernel.lds**；**ENTRY(_start)**；**_stext/_ebss** | **5.1**, **5.5** |

## 高频考点 / 面试题

1. **编译链接四阶段分别做什么？** → **cpp** 预处理 → **cc1** 编译成汇编 → **as** 成 `.o` → **ld** 符号解析与重定位（**5.1**, **demo01**）

2. **`.data` 和 `.bss` 区别？** → `.data` 有初值占映像；**.bss** 未初始化、**NOBITS**、载入 **零初始化**（**5.1**, **demo01**）

3. **强符号与弱符号冲突怎样？** → 两强 → 链接错误；一强一弱 → **强赢**；多弱 → 取其一（**5.1**, **demo03**）

4. **为什么 `gcc main.c -lm` 常写成 `-o main -lm`？** → 链接器 **从左到右** 扫描，**.a 只在已有 undefined 时** 提取成员；**`-lm` 需放在引用它的 `.o` 之后**（**5.3**）

5. **链接器会检查 `extern char *arr` 与 `char arr[]` 类型一致吗？** → **不会**，只匹配 **符号名**；运行可能 SIGSEGV（**ch04 4.2**, **5.3**）

**拓展：**

- **LD_PRELOAD** 与 **`--wrap`** 区别？（**5.4**）
- **PIC / GOT / PLT** 与延迟绑定（**5.2**）
- 循环依赖 **`--start-group`**（**5.3**）
- **Map 文件** 排查 unexpected bloat（**5.5**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[Expert C ch04](../ch04-arrays-are-not-pointers/)** 声明/定义匹配；**ch03** 声明分析 |
| 后置 | **[ch06](../ch06-POKEYOULicenseToC/)** POKEYOULicense；**ch07** 内存布局 |
| 关联 | **[Embedded C](../../05-Embedded-C-Self-Cultivation/)** 启动代码、链接脚本、裸机 |
| 全书 | **ch08** 类型与转换；**ch09** 数组再论 |

## 小节

- [5.1 函数库、链接和载入](./5.1-函数库-链接和载入.md)
- [5.2 动态链接的优点](./5.2-动态链接的优点.md)
- [5.3 函数库链接的5个特殊秘密](./5.3-函数库链接的5个特殊秘密.md)
- [5.4 警惕 Interpositioning](./5.4-警惕Interpositioning.md)
- [5.5 产生链接器报告文件](./5.5-产生链接器报告文件.md)
- [5.6 轻松一下——看看谁在说话：挑战 Turing 测验](./5.6-轻松一下看看谁在说话-挑战Turing测验.md)
