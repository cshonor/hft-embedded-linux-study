# 7.5 objdump 反汇编辅助分析

> 🔴 精读

## 本节要点

### objdump 用途

当 addr2line 不够（如优化导致行号不准）或需要理解具体指令时，用 objdump 反汇编。

```bash
# 1. 反汇编整个内核（很大）
aarch64-linux-gnu-objdump -d vmlinux > kernel.asm

# 2. 反汇编特定函数
aarch64-linux-gnu-objdump -d vmlinux | grep -A 50 '<vfs_write>:'

# 3. 反汇编模块
aarch64-linux-gnu-objdump -d my_module.ko | grep -A 30 '<my_driver_write>:'

# 4. 带源码混合反汇编（需调试符号）
aarch64-linux-gnu-objdump -S my_module.ko | grep -A 30 'my_driver_write'

# 5. 查看崩溃地址附近的指令
# pc : my_driver_write+0x3c
# 函数起始 + 0x3c 处的指令
aarch64-linux-gnu-objdump -d my_module.ko | \
    awk '/<my_driver_write>:/{found=1} found{print; if(/ret/) exit}' | \
    head -20
```

### 分析崩溃指令

```
# Oops: pc = my_driver_write+0x3c, x0 = 0 (NULL)
# 反汇编:
0000000000000000 <my_driver_write>:
       0:  a9bf7bfd    stp     x29, x30, [sp, #-16]!
       4:  910003fd    mov     x29, sp
       8:  f9000fe0    str     x0, [sp, #8]          ; 保存 arg (file *)
       c:  b9400000    ldr     w0, [x0]              ; 读 file->f_flags
      10:  110007e0    add     x0, x0, #1            ; x0 = file + 1
      ...
      3c:  b9400000    ldr     w0, [x0]              ; ← 崩溃! x0=0, 解引用 NULL
```

从反汇编可以看出 `+0x3c` 处的 `ldr w0, [x0]` 指令解引用了 x0，而 x0=0（NULL），导致 Oops。

### 6.x 新工具：decode_stacktrace.sh

```bash
# 内核提供的 Oops 解码脚本
scripts/decode_stacktrace.sh vmlinux /path/to/source < oops.log
# 自动将地址转换为源码行号
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 addr2line 有时定位的行号不准确？

> 编译器优化（如 -O2）会重排代码、内联函数、合并基本块，导致多个源码行对应同一段机器码。addr2line 报告的是 DWARF 信息记录的行号，可能与实际逻辑不完全对应。这种情况下用 `objdump -S`（源码+汇编混合）更准确。

**Q2:** 如何反汇编只看某个函数而不看整个内核？

> 用 `objdump -d vmlinux` 输出全部，然后 `grep -A N '<function>:'` 提取该函数。或用 `objdump --disassemble=function_name vmlinux` 只反汇编指定函数。模块用 `objdump -d module.ko`（文件小得多）。


**Q:** objdump -d 和 objdump -D 的区别？内核调试用哪个？

> -d 只反汇编代码段（.text），-D 反汇编所有段。内核调试用 -d（只看代码段）。常用 `aarch64-linux-gnu-objdump -d vmlinux --start-address=0xffff... --stop-address=0xffff...` 反汇编特定地址范围，配合 Oops 的 PC 地址定位。

</details>

## 交叉引用

- [05.6 ch07 addr2line](chapter-07-oops/notes/section-7-4.md)
