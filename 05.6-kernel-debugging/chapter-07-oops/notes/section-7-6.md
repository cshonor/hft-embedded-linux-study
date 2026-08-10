# 7.6 模块 Oops 的特殊处理

> 🔴 精读

## 本节要点

### 模块 Oops 的挑战

| 问题 | 说明 |
|------|------|
| 地址重定位 | 模块加载到动态地址，Oops 中的地址需要减去加载基址 |
| 符号解析 | 模块符号不在 vmlinux 中，需要单独的 .ko 文件 |
| 代码可能已卸载 | Oops 发生后模块被卸载，符号信息丢失 |
| 页可能已回收 | 模块代码页被回收后无法反汇编 |

### 模块地址解析

```bash
# 1. 获取模块加载地址
cat /proc/modules | grep my_module
# my_module 16384 1 - Live 0xffff800000100000 (O)

# 2. Oops 中的地址
# pc : ffff80000010003c [my_module]
# 函数偏移 = 0xffff80000010003c - 0xffff800000100000 = 0x3c

# 3. 用偏移在 .ko 文件中定位
aarch64-linux-gnu-objdump -d my_module.ko | grep -B2 -A1 '3c:'
# 或
aarch64-linux-gnu-addr2line -e my_module.ko 0x3c
```

### 模块 Oops 收集最佳实践

```bash
# 1. 立即保存 /proc/modules（卸载后信息丢失）
cp /proc/modules /tmp/modules_at_crash.txt

# 2. 保存 Oops 日志
dmesg > /tmp/oops.log

# 3. 保存模块文件
cp /lib/modules/$(uname -r)/my_module.ko /tmp/

# 4. 保存内核版本和配置
uname -a > /tmp/uname.txt
zcat /proc/config.gz > /tmp/kernel_config

# 5. 使用 decode_stacktrace
scripts/decode_stacktrace.sh vmlinux . < /tmp/oops.log
```

### 模块加载时保留符号

```bash
# 确保模块加载时符号被记录
insmod my_module.ko  # 正常加载
# 或强制加载带调试信息
insmod my_module.ko --verbose

# 查看已加载模块的符号
cat /proc/kallsyms | grep my_module
# ffff800000100000 t my_driver_write  [my_module]
# ffff800000100100 t my_driver_read   [my_module]
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 模块 Oops 后为什么要立即保存 /proc/modules？

> Oops 后系统可能 panic 或模块被卸载。一旦模块卸载，/proc/modules 中不再有该模块的加载地址，无法将 Oops 中的绝对地址转换为模块内偏移。因此 Oops 后第一件事是保存 /proc/modules 和 dmesg。

**Q2:** 模块代码页可能被回收是什么意思？

> 模块卸载时其代码页被释放回 buddy 分配器，可能被分配给其他用途。如果 Oops 日志中只有地址没有函数名，且模块已卸载，用 addr2line 仍然可以解析（因为 .ko 文件保留了符号），但无法在运行系统中直接反汇编该内存区域。


**Q:** 模块 Oops 中的地址如何解析？

> 模块加载到动态地址（KASLR）。需要：(1) 从 /proc/modules 获取模块加载基址；(2) Oops 中的地址减去基址得到偏移；(3) 用 addr2line -e my_module.ko 解析偏移。或者用 `echo "0xffff..." | scripts/faddr2line my_module.ko` 自动处理。

**Q:** 为什么模块符号在 /proc/kallsyms 中显示为 0x0000000000000000？

> 非 root 用户看 /proc/kallsyms 时地址为 0（kptr_restrict=1）。需要 `echo 0 > /proc/sys/kernel/kptr_restrict` 或以 root 查看。这是安全措施——防止通过 /proc/kallsyms 获取内核地址绕过 KASLR。

</details>

## 交叉引用

- [05.6 ch07 addr2line](chapter-07-oops/notes/section-7-4.md)
