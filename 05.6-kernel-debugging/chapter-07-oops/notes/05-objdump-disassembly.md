# objdump 反汇编辅助分析

> 🔴 精读

## 概念详解

### 为什么需要 objdump

addr2line 能定位源码行，但在以下场景不够用：

1. **编译优化导致行号不准**：-O2 重排代码，addr2line 可能报告错误的行
2. **需要理解具体指令**：确认是哪条指令导致崩溃（如 `ldr` 解引用 NULL）
3. **无 DEBUG_INFO 时**：只有符号表没有 DWARF 信息
4. **分析内联函数**：查看内联展开后的实际机器码

### objdump 常用命令

```bash
# 1. 反汇编特定函数
aarch64-linux-gnu-objdump -d vmlinux | grep -A 50 '<vfs_write>:'

# 2. 反汇编模块（文件小，推荐）
aarch64-linux-gnu-objdump -d my_module.ko | grep -A 30 '<my_driver_write>:'

# 3. 带源码混合反汇编（需 DEBUG_INFO）
aarch64-linux-gnu-objdump -S my_module.ko | grep -A 30 'my_driver_write'

# 4. 只反汇编指定函数
aarch64-linux-gnu-objdump --disassemble=my_driver_write my_module.ko

# 5. 反汇编特定地址范围
aarch64-linux-gnu-objdump -d vmlinux \
    --start-address=0xffff800010001000 \
    --stop-address=0xffff800010001100
```

### objdump 选项对比

| 选项 | 含义 | 适用场景 |
|------|------|---------|
| `-d` | 反汇编代码段 (.text) | **内核调试首选** |
| `-D` | 反汇编所有段 | 特殊分析 |
| `-S` | 源码+汇编混合输出 | 需要 DEBUG_INFO |
| `--disassemble=FUNC` | 只反汇编指定函数 | 精确定位 |
| `--start-address=ADDR` | 起始地址 | 地址范围反汇编 |

### 分析崩溃指令

```
# Oops 信息: pc = my_driver_write+0x3c, x0 = 0 (NULL)
# 反汇编结果:
0000000000000000 <my_driver_write>:
       0:  a9bf7bfd    stp     x29, x30, [sp, #-16]!     ; 保存帧指针
       4:  910003fd    mov     x29, sp                    ; 设置帧指针
       8:  f9000fe0    str     x0, [sp, #8]               ; 保存 arg
       c:  b9400000    ldr     w0, [x0]                   ; 读 *x0
      ...
      3c:  b9400000    ldr     w0, [x0]                   ; ← 崩溃! x0=0, 解引用 NULL
```

### 源码混合反汇编 (-S)

```bash
aarch64-linux-gnu-objdump -S my_module.ko

# 输出示例:
# 42    static ssize_t my_driver_write(struct file *f, ...) {
# 43        struct my_ctx *ctx = f->private_data;
       0:   a9bf7bfd    stp  x29, x30, [sp, #-16]!
       4:   910003fd    mov  x29, sp
       8:   f9400a00    ldr  x0, [x0, #16]    ; ctx = f->private_data
# 44        return copy_from_user(ctx->buffer, buf, len);
       c:   b9400000    ldr  w1, [x0]          ; 访问 ctx->buffer ← 崩溃!
```

### decode_stacktrace.sh

```bash
# 内核提供的 Oops 解码脚本
scripts/decode_stacktrace.sh vmlinux /path/to/source < oops.log
# 自动将地址转换为源码行号
```

### HFT 关联应用

```bash
# 检查 HFT 关键路径是否被过度优化
aarch64-linux-gnu-objdump -S my_hft_module.ko | grep -A 10 'my_busy_loop'

# 确认内存屏障指令是否正确生成
aarch64-linux-gnu-objdump -d my_hft_module.ko | grep -E 'dmb|dsb|isb'
# 应该看到 dmb ishst 等屏障指令
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 addr2line 有时定位的行号不准确？

> 编译器优化（如 -O2）会重排代码、内联函数、合并基本块，导致多个源码行对应同一段机器码。这种情况下用 `objdump -S`（源码+汇编混合）更准确。

**Q2:** 如何反汇编只看某个函数？

> 用 `objdump -d vmlinux` 输出全部，然后 `grep -A N '<function>:'` 提取。或用 `objdump --disassemble=function_name vmlinux` 只反汇编指定函数。

**Q3:** objdump -d 和 objdump -D 的区别？

> -d 只反汇编代码段（.text），-D 反汇编所有段。内核调试用 -d。常用 `--start-address` 和 `--stop-address` 反汇编特定地址范围。

**Q4:** `objdump -S` 和 `objdump -d` 哪个更适合调试？

> 调试首选 `-S`（源码混合），直接显示哪行 C 代码对应哪条指令。但 `-S` 需要 DEBUG_INFO。HFT 模块建议始终带 DEBUG_INFO 编译。

**Q5:** 如何确认编译器是否将关键的忙等待循环优化掉了？

> 用 `objdump -S my_module.ko` 查看该函数的汇编。如果循环体只剩几条指令且没有预期的内存访问，可能被优化掉了。解决：用 `barrier()` 或 `READ_ONCE()` 阻止编译器优化。

</details>

## 交叉引用

- [05.6 ch07 addr2line](../../chapter-07-oops/notes/04-addr2line.md)
- [05.6 ch07 寄存器转储解读](../../chapter-07-oops/notes/02-register-dump.md)
- [05.6 ch07 模块 Oops 特殊处理](../../chapter-07-oops/notes/06-module-oops.md)
