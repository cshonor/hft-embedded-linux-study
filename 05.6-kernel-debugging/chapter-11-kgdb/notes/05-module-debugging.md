# 调试内核模块 (loadable module)

> 🔴 精读

## 概念详解

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

# 或用 getmodinfo.sh 脚本
# 在目标机上运行，输出 add-symbol-file 命令
cat > /tmp/getmodinfo.sh << 'EOF'
#!/bin/bash
MOD=$1
BASE=$(cat /sys/module/$MOD/sections/.text)
DATA=$(cat /sys/module/$MOD/sections/.data)
BSS=$(cat /sys/module/$MOD/sections/.bss)
echo "add-symbol-file $MOD.ko $BASE -s .data $DATA -s .bss $BSS"
EOF
chmod +x /tmp/getmodinfo.sh
/tmp/getmodinfo.sh my_module
# 输出: add-symbol-file my_module.ko 0xffff... -s .data 0xffff... -s .bss 0xffff...
```

### 调试模块加载过程

```gdb
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

### 调试模块卸载过程

```gdb
# 在 module_exit 函数设断点
(gdb) break my_module_exit
(gdb) continue
# rmmod 时命中断点

# 或在 free_module 设断点
(gdb) break free_module
(gdb) continue
```

### HFT 关联应用

```bash
# HFT 模块调试完整流程

# 1. 目标机: 加载模块并进入 KGDB
insmod my_hft_module.ko
echo g > /proc/sysrq-trigger

# 2. 开发机: GDB 连接
aarch64-linux-gnu-gdb vmlinux
(gdb) target remote /dev/ttyUSB0

# 3. 获取模块段地址 (从目标机串口获取)
# cat /sys/module/my_hft_module/sections/.text
# cat /sys/module/my_hft_module/sections/.data

# 4. 加载模块符号
(gdb) add-symbol-file my_hft_module.ko 0xffff800000100000 \
      -s .data 0xffff800000104000

# 5. 设断点
(gdb) break on_trade_signal
(gdb) break on_order_fill
(gdb) continue

# 6. 触发交易，断点命中
# 7. 分析
(gdb) bt
(gdb) print *order
(gdb) print order_book->count
```

### 模块调试注意事项

| 问题 | 说明 | 解决方案 |
|------|------|---------|
| KASLR 导致地址变化 | 每次加载地址不同 | 每次重新获取地址 |
| 模块卸载后符号消失 | .ko 文件仍可用 | 保存 .ko 文件 |
| 内联函数无符号 | addr2line 需要 -i | 用 `info line` 查看 |
| 优化导致变量丢失 | -O2 优化掉局部变量 | 用 -O0 或 -Og 编译 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么调试内核模块需要 `add-symbol-file`？

> 模块在加载时被重定位到动态地址。GDB 只有模块的 .ko 文件（未重定位），不知道模块在内核中的实际加载位置。`add-symbol-file` 告诉 GDB 模块各段的实际地址，使 GDB 能正确映射符号到地址。

**Q2:** 用 KGDB 调试内核模块时，模块的符号如何加载到 GDB？

> (1) 在目标机 `cat /proc/modules` 获取模块加载地址；(2) 在 GDB 中 `add-symbol-file my_module.ko 0xffff...`；(3) 之后可以按源码设断点。注意 KASLR 下模块地址每次不同。

**Q3:** 如何调试模块的 init 函数（加载时执行的函数）？

> (1) 在 `do_one_initcall` 设断点；(2) `continue` 等待模块加载命中；(3) 获取模块基地址 `print mod->core_layout.base`；(4) `add-symbol-file` 加载符号；(5) 在 init 函数设断点；(6) `continue` 命中 init 断点。

**Q4:** 模块用 -O2 编译后，KGDB 调试时变量不可见怎么办？

> -O2 优化可能将局部变量优化到寄存器或直接消除。解决：(1) 模块用 -Og 或 -O0 编译（调试模式）；(2) 关键变量加 `volatile`（不推荐在内核中用）；(3) 用 `print $寄存器名` 直接读寄存器。

**Q5:** KASLR 对模块调试有什么影响？

> KASLR 使模块每次加载到不同地址。每次进入 KGDB 后需要重新获取模块基地址并重新 `add-symbol-file`。可以用 `nokaslr` 内核参数禁用 KASLR 简化调试（仅开发环境）。

</details>

## 交叉引用

- [05.6 ch11 断点/单步/查看变量](chapter-11-kgdb/notes/04-breakpoints-variables.md)
- [05.6 ch07 模块 Oops 特殊处理](chapter-07-oops/notes/06-module-oops.md)
- [05.6 ch11 KGDB 原理与架构](chapter-11-kgdb/notes/01-kgdb-architecture.md)
