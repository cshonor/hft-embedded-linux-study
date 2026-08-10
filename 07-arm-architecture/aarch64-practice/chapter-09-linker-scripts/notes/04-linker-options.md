# 9.4 常用链接器选项

> 来源：§9.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GCC/LD 常用链接选项：指定链接脚本、输出映射文件、垃圾回收段、静态/动态链接、符号定义等。

## 常用选项速查表

| 选项 | 作用 | 示例 |
|------|------|------|
| `-T script.ld` | 指定链接脚本 | `gcc -T custom.ld -o app *.o` |
| `-Wl,-Map=map.txt` | 输出链接映射文件 | 记录每个段和符号的地址 |
| `-Wl,--gc-sections` | 删除未引用的段 | 配合 `-ffunction-sections` |
| `-nostdlib` | 不链接标准库 | 裸机开发必用 |
| `-nostartfiles` | 不链接启动文件（crt0） | 裸机自定义入口 |
| `-static` | 静态链接 | 避免动态链接开销 |
| `-Wl,--defsym` | 定义符号 | `--defsym=DEBUG=1` |
| `-Wl,-Ttext=0x80000` | 指定 .text 起始地址 | 不需写完整脚本 |
| `-Wl,--build-id` | 生成 build ID | 调试标识 |
| `-Wl,-z,noexecstack` | 标记栈不可执行 | 安全加固 |

> **注意**：通过 gcc 传给 ld 的选项要加 `-Wl,` 前缀。直接调用 `ld` 则不需要。

## 选项详解

### 1. -T 指定链接脚本

```bash
# 完整自定义脚本
gcc -T my_link.ld -o app a.o b.o

# 直接指定地址（简易脚本）
gcc -Wl,-Ttext=0x80000 -Wl,-Tdata=0x90000 -o app *.o
```

### 2. -Map 输出映射文件

```bash
gcc -Wl,-Map=output.map -o app *.o

# map 文件内容示例：
#   .text 0x80000:        ← 段名 + 起始地址
#     *(.text)
#     a.o(.text)          ← 输入文件贡献
#       main              0x80000  0x40  ← 符号+地址+大小
#     b.o(.text)
#       do_work           0x80040  0x20
```

映射文件的用途：
- 确认段布局和地址分配
- 查找符号的最终地址
- 分析代码体积（哪个函数最大）
- 调试段重叠问题

### 3. --gc-sections 垃圾回收

```bash
# 编译：每函数独立段
gcc -ffunction-sections -fdata-sections -c a.c

# 链接：删除未引用的段
gcc -Wl,--gc-sections -o app a.o

# 对比效果
size app_normal       # text = 0x2000
size app_gc_sections  # text = 0x1500  ← 未引用函数被删
```

**工作原理**：链接器从 ENTRY 符号出发，做可达性分析，删除不可达的段。需要 `-ffunction-sections` 让每个函数独立成段，否则一个段内只要有一个函数被引用，整个段都保留。

| 选项 | 作用 |
|------|------|
| `-ffunction-sections` | 每个函数 → 独立的 .text.function_name 段 |
| `-fdata-sections` | 每个全局变量 → 独立的 .data.var_name 段 |
| `-Wl,--gc-sections` | 删除不可达段（不被 ENTRY 或任何引用触达） |
| `KEEP(*(.init))` | 链接脚本中防止误删 |

### 4. -nostdlib / -nostartfiles

| 选项 | 不链接的内容 | 适用场景 |
|------|------------|---------|
| `-nostdlib` | 标准库 + 启动文件 | 完全裸机，自己实现一切 |
| `-nostartfiles` | 只跳过启动文件（crt0/crti/crtn） | 保留 libc 但自定义入口 |
| `-nodefaultlibs` | 只跳过默认库 | 保留启动文件但自己选库 |

```bash
# 裸机开发
gcc -nostdlib -T linker.ld -o baremetal.elf start.o main.o

# 只自定义入口，用 libc
gcc -nostartfiles -o app my_start.o main.o -lc
```

### 5. -static 静态链接

```bash
# 动态链接（默认）
gcc -o app_dyna a.c           # 依赖 libxxx.so
ldd app_dyna                  # 列出依赖的 .so

# 静态链接
gcc -static -o app_static a.c  # 所有代码打包进可执行文件
ldd app_static                 # "not a dynamic executable"
```

| 特性 | 动态链接 | 静态链接 |
|------|---------|---------|
| 文件大小 | 小 | 大 |
| 启动速度 | 慢（加载 .so） | 快 |
| 内存占用 | 共享 .so | 每份独立 |
| 更新 | 换 .so 即可 | 需重新编译 |
| HFT 适用 | ✗（不确定延迟） | ✓（确定性好） |

### 6. 符号定义选项

```bash
# 命令行定义符号
gcc -Wl,--defsym=CONFIG_DEBUG=1 -o app *.o

# 链接脚本中引用
# if (DEFINED(CONFIG_DEBUG)) { ... }
```

### 7. 安全相关选项

| 选项 | 作用 |
|------|------|
| `-Wl,-z,noexecstack` | 标记栈不可执行（防 shellcode 注入） |
| `-Wl,-z,relro,-z,now` | 完整 RELRO（GOT 只读） |
| `-Wl,--build-id` | 唯一标识，便于调试 |
| `-pie -fPIE` | 位置无关可执行文件 |

## GCC vs LD 选项传递

```bash
# 方式1：通过 gcc 传递（推荐，gcc 自动加必要库）
gcc -Wl,--gc-sections -Wl,-Map=out.map -o app *.o

# 方式2：直接用 ld（需手动指定库和路径）
ld --gc-sections -Map=out.map -o app *.o -lc -lgcc
```

## HFT 关联

- **`--gc-sections` 配合 `-ffunction-sections`** → 删除死代码，减小 icache 压力
- **`-static` 静态链接** → 避免运行时动态链接的不确定延迟
- **`-Wl,-z,noexecstack`** → 安全加固，HFT 交易系统也要安全
- **精确控制段地址** → 热路径代码放 SRAM，通过 `-T` 脚本控制

## 自测题

1. `--gc-sections` 需要配合什么编译选项？
<details><summary>答案</summary>
`-ffunction-sections`（每个函数独立成段）和 `-fdata-sections`（每个全局变量独立成段）。否则函数在同一个段中，只要有一个被引用，整个段都保留，GC 无法删除其中的死函数。
</details>

2. `-Map=output.map` 输出的文件有什么用？
<details><summary>答案</summary>
链接映射文件，记录每个段的起始地址、大小，以及每个符号的最终地址。用途：（1）确认段布局正确（2）查找符号地址（3）分析代码体积找最大的函数（4）调试段重叠问题。
</details>

3. 裸机开发为什么要用 `-nostdlib`？如果只用 `-nostartfiles` 有什么不同？
<details><summary>答案</summary>
`-nostdlib` 不链接标准库和启动文件，因为裸机无 OS，标准库依赖的系统调用（malloc/printf→syscall）不可用。`-nostartfiles` 只跳过启动文件（crt0），但保留 libc，适合想用 printf/malloc 但自定义 _start 入口的场景。
</details>

4. `-static` 和动态链接相比，对 HFT 有什么优势？
<details><summary>答案</summary>
（1）启动确定性：不需要运行时加载 .so，无动态链接开销（2）无 PLT/GOT 间接跳转：函数直接调用而非通过 GOT 跳转表（3）地址固定：函数地址在编译时确定，利于 cache 预取。代价是文件大、不能共享库代码。
</details>

5. 如何通过链接器选项在命令行定义一个宏 `DEBUG=1`？
<details><summary>答案</summary>
`gcc -Wl,--defsym=DEBUG=1`。但注意这是链接时符号，值为整数地址。如果需要条件编译宏，应在编译时用 `-DDEBUG=1`。`--defsym` 用于定义链接脚本可检测的符号。
</details>

## 参考与延伸

- 原书 §9.4
- [9.5 分析链接结果](05-analyze-output.md)
- `man ld` 查看完整选项
