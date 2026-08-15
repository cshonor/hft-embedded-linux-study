# addr2line 定位源码行

> 🔴 精读

## 概念详解

### addr2line 是什么

`addr2line` 是 GNU Binutils 工具，将机器地址映射到源码文件和行号。它依赖 DWARF 调试信息（`CONFIG_DEBUG_INFO=y` 编译时生成），是分析 Oops 日志的核心工具。

### 工作原理

```
Oops 地址 (如 vfs_write+0xf4)
    ↓
addr2line 查询 ELF 文件的 DWARF 信息
    ↓
DWARF .debug_line 段记录了 "地址 ↔ 源码行" 映射
    ↓
输出: /path/to/fs/read_write.c:623
```

### 基本用法

```bash
# 1. 对于内核内置函数 (使用 vmlinux)
aarch64-linux-gnu-addr2line -e vmlinux -f vfs_write+0xf4
# 输出: vfs_write  /path/to/fs/read_write.c:623

# 2. 对于模块函数 (使用 .ko 文件)
aarch64-linux-gnu-addr2line -e my_module.ko -f my_driver_write+0x3c
# 输出: my_driver_write  /path/to/my_driver.c:45

# 3. 使用绝对地址 (需要从 /proc/modules 计算偏移)
cat /proc/modules | grep my_module
# my_module 16384 1 - Live 0xffff800000100000
# 偏移 = Oops绝对地址 - 加载基址
aarch64-linux-gnu-addr2line -e my_module.ko -f 0x3c

# 4. -i 选项显示内联函数
aarch64-linux-gnu-addr2line -e vmlinux -f -i vfs_write+0xf4
# 可能输出多层内联展开
```

### addr2line 常用选项

| 选项 | 含义 | 用途 |
|------|------|------|
| `-e FILE` | 指定 ELF 文件 | vmlinux 或 .ko |
| `-f` | 显示函数名 | 确认地址所在函数 |
| `-i` | 展开内联函数 | 查看内联调用链 |
| `-p` | pretty-print | 输出更易读 |
| `-a` | 显示地址 | 调试用途 |

### 使用 faddr2line 脚本

```bash
# 内核源码中的工具脚本 (推荐)
cd /path/to/linux-source
echo "vfs_write+0xf4" | scripts/faddr2line vmlinux
# 输出更详细，自动展开内联

# 模块也支持
echo "my_driver_write+0x3c" | scripts/faddr2line my_module.ko
```

### faddr2line vs addr2line

| 特性 | addr2line | faddr2line |
|------|-----------|------------|
| 输入格式 | 绝对地址或符号+偏移 | 函数名+偏移（Oops 格式） |
| 内联展开 | 需要 `-i` 选项 | 自动展开 |
| 函数大小 | 不显示 | 显示 `/总大小` |
| 符号查找 | 手动 | 自动 |
| 推荐度 | 基础工具 | **首选** |

### 实际操作流程

```bash
# 1. 保存 Oops 日志
dmesg > oops.log

# 2. 确认内核版本和编译路径
uname -r  # 6.1.63-v8+

# 3. 获取带调试符号的 vmlinux (CONFIG_DEBUG_INFO=y)
ls vmlinux  # 在内核源码根目录

# 4. 对每个 Call Trace 地址运行 addr2line
aarch64-linux-gnu-addr2line -e vmlinux -f -i vfs_write+0xf4
aarch64-linux-gnu-addr2line -e my_module.ko -f -i my_driver_write+0x3c

# 5. 查看源码
less -N /path/to/my_driver.c  # 跳到对应行
```

### addr2line 输出 "??" 的原因

| 原因 | 解决方案 |
|------|---------|
| vmlinux 没有编译 DEBUG_INFO | 重新编译 `CONFIG_DEBUG_INFO=y` |
| 优化导致函数内联 | 用 `-i` 选项展开内联 |
| 地址是模块地址但用了 vmlinux | 改用 .ko 文件 |
| 位置信息被 strip | 使用未 strip 的 vmlinux |

### HFT 关联应用

```bash
# HFT 模块崩溃快速定位脚本
#!/bin/bash
OOPS_LOG=$1
VMLINUX=/path/to/vmlinux
grep -oP '\w+\+0x\w+/\w+' "$OOPS_LOG" | while read addr; do
    echo "=== $addr ==="
    echo "$addr" | scripts/faddr2line "$VMLINUX" 2>/dev/null
done
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** addr2line 为什么需要带调试符号的 vmlinux？

> addr2line 依赖 DWARF 调试信息将机器地址映射到源码行号。调试信息在 `CONFIG_DEBUG_INFO=y` 时生成，存储在 vmlinux 中。没有调试符号的 vmlinux 无法做地址到行号的映射。

**Q2:** 模块函数的 addr2line 和内置函数有什么不同？

> 内置函数直接用 vmlinux 作为输入文件。模块函数需要用模块的 .ko 文件作为输入，且模块加载到内核时有重定位偏移。addr2line 用函数名+偏移时会自动解析符号表。

**Q3:** addr2line 输出 "??" 或 "0" 是什么原因？

> (1) vmlinux 没有编译 DEBUG_INFO；(2) 优化导致函数内联；(3) 地址是模块地址但用了 vmlinux 作为输入；(4) 位置信息被 strip。解决：确保 CONFIG_DEBUG_INFO=y，模块用 .ko 文件，用 faddr2line 脚本。

**Q4:** faddr2line 和 addr2line 的区别？

> faddr2line 是内核源码中的脚本（scripts/faddr2line），接受 "function+offset" 格式，内部调用 addr2line + objdump。优势：自动处理符号查找和偏移计算，支持内联函数展开。

**Q5:** `-i` 选项的作用是什么？

> `-i` 选项展开内联函数。编译器可能将小函数内联到调用者中，addr2line 不加 `-i` 只显示外层函数的行号。加 `-i` 后会显示完整的内联调用链。faddr2line 默认启用内联展开。

</details>

## 交叉引用

- [05.6 ch07 寄存器转储解读](../../chapter-07-oops/notes/02-register-dump.md)
- [05.6 ch07 栈回溯分析](../../chapter-07-oops/notes/03-call-trace-analysis.md)
- [05.6 ch07 objdump 反汇编](../../chapter-07-oops/notes/05-objdump-disassembly.md)
- [05.6 ch07 模块 Oops 特殊处理](../../chapter-07-oops/notes/06-module-oops.md)
