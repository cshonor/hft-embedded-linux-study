# 7.4 addr2line 定位源码行

> 🔴 精读

## 本节要点

### addr2line 基本用法

```bash
# 1. 从 Oops 中获取崩溃地址
# pc : my_driver_write+0x3c/0x100 [my_module]

# 2. 对于内核内置函数 (非模块)
aarch64-linux-gnu-addr2line -e vmlinux -f vfs_write+0xf4
# 输出:
# vfs_write
# /path/to/fs/read_write.c:623

# 3. 对于模块函数，需要模块的 .o 文件
aarch64-linux-gnu-addr2line -e my_module.o -f my_driver_write+0x3c
# 输出:
# my_driver_write
# /path/to/my_driver.c:45

# 4. 使用绝对地址 (从 /proc/modules 获取偏移)
cat /proc/modules | grep my_module
# my_module 16384 1 - Live 0xffff800000100000

# Oops 中的绝对地址:
# pc : ffff80000010003c
# 偏移 = 0xffff80000010003c - 0xffff800000100000 = 0x3c
aarch64-linux-gnu-addr2line -e my_module.ko -f 0x3c
```

### 实际操作流程

```bash
# 1. 保存 Oops 日志
dmesg > oops.log

# 2. 确认内核版本
uname -r  # 6.1.63-v8+

# 3. 获取带调试符号的 vmlinux
# (编译时需 CONFIG_DEBUG_INFO=y)
ls vmlinux  # 在内核源码根目录

# 4. 对每个 Call Trace 地址运行 addr2line
# 内核函数
aarch64-linux-gnu-addr2line -e vmlinux vfs_write+0xf4
aarch64-linux-gnu-addr2line -e vmlinux ksys_write+0x74

# 模块函数 (需要模块的 .ko 文件)
aarch64-linux-gnu-addr2line -e my_module.ko my_driver_write+0x3c

# 5. 查看源码
# addr2line 输出文件:行号，直接查看
less -N /path/to/my_driver.c  # 跳到对应行
```

### 使用 faddr2line 脚本

```bash
# 内核源码中的工具脚本
cd /path/to/linux-source
echo "vfs_write+0xf4" | scripts/faddr2line vmlinux
# 输出更详细:
# vfs_write+0xf4/0x2b0:
# vfs_write (fs/read_write.c:623)

# 模块也支持
echo "my_driver_write+0x3c" | scripts/faddr2line my_module.ko
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** addr2line 为什么需要带调试符号的 vmlinux？

> addr2line 依赖 DWARF 调试信息将机器地址映射到源码行号。调试信息在 `CONFIG_DEBUG_INFO=y` 时生成，存储在 vmlinux 中。没有调试符号的 vmlinux 无法做地址到行号的映射。

**Q2:** 模块函数的 addr2line 和内置函数有什么不同？

> 内置函数直接用 vmlinux 作为输入文件。模块函数需要用模块的 .ko 文件作为输入，且模块加载到内核时有重定位偏移。addr2line 用函数名+偏移（如 `my_driver_write+0x3c`）时会自动解析符号表，不需要计算加载偏移。


**Q:** addr2line 输出 "??" 或 "0" 是什么原因？

> (1) vmlinux 没有编译 DEBUG_INFO；(2) 优化导致函数内联（addr2line 找到的是内联展开位置而非源码函数）；(3) 地址是模块地址但用了 vmlinux 作为输入；(4) 位置信息被 strip。解决：确保 CONFIG_DEBUG_INFO=y，模块用 .ko 文件，用 faddr2line 脚本替代直接 addr2line。

**Q:** faddr2line 和 addr2line 的区别？

> faddr2line 是内核源码中的脚本（scripts/faddr2line），接受 "function+offset" 格式（如 "vfs_write+0xf4"），内部调用 addr2line + objdump。优势：自动处理符号查找和偏移计算，支持内联函数展开。addr2line 需要手动计算绝对地址。

</details>

## 交叉引用

- [05.6 ch07 objdump](chapter-07-oops/notes/section-7-5.md)
