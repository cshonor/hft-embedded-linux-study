# 模块 Oops 的特殊处理

> 🔴 精读

## 概念详解

### 模块 Oops 的挑战

| 问题 | 说明 | 影响 |
|------|------|------|
| 地址重定位 | 模块加载到动态地址，KASLR 使地址每次不同 | Oops 地址需要减去加载基址 |
| 符号解析 | 模块符号不在 vmlinux 中 | addr2line 需要用 .ko 文件 |
| 代码可能已卸载 | Oops 发生后模块可能被卸载 | 符号信息丢失，无法分析 |
| 页可能已回收 | 模块代码页被释放后无法反汇编 | 无法在线反汇编 |

### 模块地址解析

```bash
# 1. 获取模块加载地址（关键步骤！）
cat /proc/modules | grep my_module
# my_module 16384 1 - Live 0xffff800000100000 (O)

# 2. Oops 中的地址: pc = ffff80000010003c [my_module]
# 偏移 = 0xffff80000010003c - 0xffff800000100000 = 0x3c

# 3. 用偏移在 .ko 文件中定位
aarch64-linux-gnu-addr2line -e my_module.ko 0x3c
# 或直接用符号+偏移
aarch64-linux-gnu-addr2line -e my_module.ko my_driver_write+0x3c

# 4. 用 faddr2line（更方便）
echo "my_driver_write+0x3c" | scripts/faddr2line my_module.ko
```

### /proc/modules 输出解读

```
my_module 16384 1 - Live 0xffff800000100000 (O)
|         |     | |  |    |                  |
模块名     大小  引用 - 状态  加载地址           标志(O=树外)
```

### /proc/kallsyms 中的模块符号

```bash
# 查看已加载模块的符号
cat /proc/kallsyms | grep my_module
# ffff800000100000 t my_driver_write  [my_module]
# ffff800000100100 t my_driver_read   [my_module]

# 非 root 用户看到地址为 0 (kptr_restrict=1)
# 解决: 以 root 运行或降低 kptr_restrict
echo 0 > /proc/sys/kernel/kptr_restrict
```

### kptr_restrict 安全机制

| 值 | 行为 | 适用场景 |
|----|------|---------|
| 0 | 显示真实地址 | 调试环境 |
| 1 | 非 root 看到 0 | 生产环境（默认） |
| 2 | 所有人看到 0 | 高安全环境 |

### 模块 Oops 收集最佳实践

```bash
#!/bin/bash
# 模块 Oops 信息收集脚本（Oops 发生后立即运行！）

# 1. 立即保存 /proc/modules（卸载后信息丢失）
cp /proc/modules /tmp/modules_at_crash.txt

# 2. 保存 Oops 日志
dmesg > /tmp/oops.log

# 3. 保存模块文件
cp /lib/modules/$(uname -r)/my_module.ko /tmp/

# 4. 保存内核版本和配置
uname -a > /tmp/uname.txt
zcat /proc/config.gz > /tmp/kernel_config

# 5. 保存 kallsyms
cat /proc/kallsyms > /tmp/kallsyms_at_crash.txt

# 6. 使用 decode_stacktrace 解码
scripts/decode_stacktrace.sh vmlinux . < /tmp/oops.log > /tmp/oops_decoded.txt
```

### 模块编译注意事项

```makefile
# Makefile 中确保调试信息
obj-m += my_module.o
ccflags-y += -g -O0    # -O0 禁用优化，便于调试
# 或 -Og (GCC 4.8+，优化但不影响调试)
```

### 模块 vs 内置代码 Oops 分析对比

| 方面 | 内置代码 | 模块 |
|------|---------|------|
| addr2line 输入 | vmlinux | .ko 文件 |
| KASLR 影响 | 需要修正偏移 | 符号+偏移不受影响 |
| 符号可见性 | 始终可见 | 卸载后消失 |
| Call Trace 标记 | 无特殊标记 | `[module_name]` |

### HFT 关联应用

```bash
# HFT 模块崩溃后的一键收集脚本
#!/bin/bash
MODULE=my_hft_module
DATE=$(date +%Y%m%d_%H%M%S)
DIR=/tmp/oops_${MODULE}_${DATE}
mkdir -p "$DIR"

cp /proc/modules "$DIR/"
dmesg > "$DIR/dmesg.log"
cat /proc/kallsyms > "$DIR/kallsyms.txt"
cp /lib/modules/$(uname -r)/${MODULE}.ko "$DIR/" 2>/dev/null
uname -a > "$DIR/uname.txt"
zcat /proc/config.gz > "$DIR/kernel_config" 2>/dev/null

echo "Oops 信息已收集到 $DIR"
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 模块 Oops 后为什么要立即保存 /proc/modules？

> Oops 后系统可能 panic 或模块被卸载。一旦模块卸载，/proc/modules 中不再有该模块的加载地址，无法将 Oops 中的绝对地址转换为模块内偏移。

**Q2:** 模块代码页可能被回收是什么意思？

> 模块卸载时其代码页被释放回 buddy 分配器。如果 Oops 日志中只有地址没有函数名，且模块已卸载，用 addr2line 仍然可以解析（因为 .ko 文件保留了符号），但无法在运行系统中直接反汇编。

**Q3:** 模块 Oops 中的地址如何解析？

> 模块加载到动态地址（KASLR）。需要：(1) 从 /proc/modules 获取模块加载基址；(2) Oops 地址减去基址得到偏移；(3) 用 addr2line -e my_module.ko 解析偏移。或直接用符号+偏移。

**Q4:** 为什么模块符号在 /proc/kallsyms 中显示为 0x0000000000000000？

> 非 root 用户看 /proc/kallsyms 时地址为 0（kptr_restrict=1）。需要 `echo 0 > /proc/sys/kernel/kptr_restrict` 或以 root 查看。这是安全措施——防止绕过 KASLR。

**Q5:** 模块 Oops 后模块已被卸载，还能分析吗？

> 可以，前提是保存了 .ko 文件和 /proc/modules 快照。用 .ko 文件做 addr2line/objdump，用 /proc/modules 的加载地址计算偏移。但如果没保存 /proc/modules，就不知道加载基址。

</details>

## 交叉引用

- [05.6 ch07 addr2line](chapter-07-oops/notes/04-addr2line.md)
- [05.6 ch07 objdump 反汇编](chapter-07-oops/notes/05-objdump-disassembly.md)
- [05.6 ch11 KGDB 调试模块](chapter-11-kgdb/notes/01-kgdb-setup.md)
