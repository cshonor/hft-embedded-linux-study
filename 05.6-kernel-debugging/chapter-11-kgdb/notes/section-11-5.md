# 11.5 调试内核模块 (loadable module)

> 🔴 精读

## 本节要点

### 模块符号加载

```bash
# 1. 在目标机上获取模块加载地址
cat /proc/modules | grep my_module
# my_module 16384 1 - Live 0xffff800000100000 (O)

# 2. 获取模块各段地址 (更精确)
cat /sys/module/my_module/sections/.text
# 0xffff800000100000
cat /sys/module/my_module/sections/.data
# 0xffff800000104000
cat /sys/module/my_module/sections/.bss
# 0xffff800000105000
```

```gdb
# 3. 在 GDB 中加载模块符号
(gdb) add-symbol-file my_module.ko 0xffff800000100000 \
      -s .data 0xffff800000104000 \
      -s .bss 0xffff800000105000
# 确认加载

# 4. 现在可以设置模块断点
(gdb) break my_driver_write
(gdb) break my_module.c:45
(gdb) continue
```

### 自动化模块符号加载

```bash
# 使用内核提供的 gdb 脚本
(gdb) source /path/to/linux/scripts/gdb/vmlinux-gdb.py
(gdb) lx-module-load my_module
# 自动获取地址并加载符号
```

### 调试模块加载过程

```bash
# 在 do_one_initcall 设断点，捕获模块初始化
(gdb) break do_one_initcall
(gdb) continue
# 模块加载时会命中断点
# 此时模块符号还未加载，先获取地址
(gdb) print mod->name
(gdb) print mod->core_layout.base
# 然后加载符号
(gdb) add-symbol-file my_module.ko <base_addr>
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么调试内核模块需要 `add-symbol-file`？

> 模块在加载时被重定位到动态地址。GDB 只有模块的 .ko 文件（未重定位），不知道模块在内核中的实际加载位置。`add-symbol-file` 告诉 GDB 模块各段的实际地址，使 GDB 能正确映射符号到地址。


**Q:** 用 KGDB 调试内核模块时，模块的符号如何加载到 GDB？

> (1) 在目标机 `cat /proc/modules` 获取模块加载地址；(2) 在 GDB 中 `add-symbol-file my_module.ko 0xffff...`（指定 .text 节地址）；(3) 之后可以按源码设断点。注意 KASLR 下模块地址每次不同，需每次重新加载。

</details>

## 交叉引用

- [05.6 ch07 module Oops](chapter-07-oops/notes/section-7-6.md)
