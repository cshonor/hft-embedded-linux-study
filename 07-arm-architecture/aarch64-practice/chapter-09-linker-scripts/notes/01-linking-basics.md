# 9.1 链接基本概念

> 来源：§9.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

链接器（Linker, `ld`）的作用：把多个 `.o` 目标文件（Relocatable Object File）合并为一个可执行文件或共享库，解析符号引用，分配最终地址。链接是编译流程的最后一步：源码 → 预处理 → 编译 → 汇编 → **链接**。

## 链接的三个核心任务

| 任务 | 说明 | 做错了的后果 |
|------|------|------------|
| **符号解析** | 把 .o 文件中的未定义符号引用（如 `printf`）绑定到另一个 .o 的定义 | `undefined reference` 错误 |
| **段合并** | 把多个 .o 的同名段（.text/.data/.bss/.rodata）合并为一个连续段 | 内存碎片、属性冲突 |
| **地址分配** | 给每个段和符号分配最终的虚拟地址 | 程序无法执行 |

### 符号解析过程

```
# a.c 调用了 b.c 的函数
# a.o 的符号表：
#     U do_work        ← U = Undefined，需要链接器解析
#     T main           ← T = Text，已定义
#
# b.o 的符号表：
#     T do_work         ← T = Text，已定义
#
# 链接后 a.o 的 U do_work → 绑定到 b.o 的 T do_work
```

### 段合并规则

```
输入文件：        合并后：
  a.o .text ──┐    .text (a.text + b.text)
  b.o .text ──┘    .data (a.data + b.data)
  a.o .data ──┐    .bss  (a.bss  + b.bss)
  b.o .data ──┘    .rodata(a.rodata + b.rodata)
```

同名段按**输入顺序**拼接，最终地址从起始地址开始递增分配。

### 强符号与弱符号

| 类型 | 定义方式 | 重复定义时的行为 |
|------|---------|---------------|
| 强符号 | 普通全局变量 `int x = 1;` | 多个强符号 → 链接报错 |
| 弱符号 | `__attribute__((weak))` | 强符号优先，多个弱符号取第一个 |
| 公共符号 | 非标准 C，GCC 扩展 `int x;`（无初值） | 取最大尺寸 |

```c
/* file1.c */
int x = 1;              /* 强符号 */

/* file2.c */
__attribute__((weak)) int x = 2;  /* 弱符号，被强符号覆盖 */

/* 链接后 x = 1（强符号胜出） */
```

## 链接的两种模式

| 模式 | 说明 | 特点 |
|------|------|------|
| 静态链接 | 所有代码在链接时复制到可执行文件 | 文件大但独立，启动快 |
| 动态链接 | 运行时由 ld-linux 加载 .so 共享库 | 文件小，但启动有开销 |

裸机和 HFT 场景通常用**静态链接**，避免动态链接的不确定延迟。

## 默认链接脚本

不指定 `-T` 时，`ld` 使用内置默认脚本：

```bash
# 查看默认链接脚本
ld --verbose | head -80

# GCC 实际使用的链接命令和脚本
gcc -Wl,--verbose 2>&1 | head -80
```

默认脚本把 `.text` 放到 `0x400000`（Linux 用户空间），`.data`/`.bss` 紧随其后，入口地址通常为 `_start`。

## LTO 链接时优化

| 特性 | 传统链接 | LTO 链接 |
|------|---------|---------|
| 优化范围 | 单个 .o 文件内 | 跨文件跨模块 |
| 内联 | 同文件可见 | 跨文件内联 |
| 死代码消除 | 整段保留或删除 | 函数级精确删除 |
| 编译时间 | 快 | 慢（需重新优化） |

```bash
# 启用 LTO
gcc -flto -O2 -o app a.c b.c
```

## HFT 关联

- **段布局影响 cache 行为** → 热路径代码集中放置减少 icache miss，冷路径放远
- **链接脚本控制内存布局** → 把代码放在特定内存区域（如 SRAM vs DRAM）
- **静态链接消除动态链接延迟** → 启动后无不确定的库加载开销
- **`--gc-sections` 删除死代码** → 减小 icache 占用，降低 miss 率

## 自测题

1. 链接器为什么要合并同名段？
<details><summary>答案</summary>
（1）统一管理内存属性（.text 只读可执行、.data 可读写）（2）减少内存碎片和页表条目（3）同段内符号引用可用相对地址（PC 相对偏移更小）
</details>

2. 两个 .o 文件都定义了 `int x = 1`，链接会怎样？
<details><summary>答案</summary>
链接报错 `multiple definition of 'x'`。解决方案：一个改为 `extern int x`（声明引用），或用 `static` 限制作用域到文件内，或用 `__attribute__((weak))` 让其中一个成为弱符号。
</details>

3. `ld --verbose` 输出的是什么？
<details><summary>答案</summary>
默认链接脚本（Default linker script）。显示默认的段布局（.text/.data/.bss 地址）、入口地址（ENTRY）、内存区域定义等。用来对比自定义脚本的差异。
</details>

4. 弱符号 `__attribute__((weak))` 有什么实际用途？
<details><summary>答案</summary>
（1）库函数提供默认实现，用户可覆盖（2）可选功能：有就用，没有也不报错（3）内核启动：`__weak` 的 `setup_arch()` 允许不同架构覆盖默认实现。
</details>

5. LTO 和普通链接相比，最大的优势是什么？
<details><summary>答案</summary>
跨文件内联和跨文件死代码消除。比如 a.c 调用 b.c 的 `hot_func()`，LTO 可以把 `hot_func()` 内联到 a.c 的调用点，消除函数调用开销。普通链接做不到，因为编译只能看到单个 .o。
</details>

## 参考与延伸

- 原书 §9.1
- [9.2 链接脚本语法](02-linker-script-syntax.md)
- CSAPP 第7章「链接」深入讲符号解析和重定位
