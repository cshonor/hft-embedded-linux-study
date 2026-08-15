# 5.5 kmemleak：内核内存泄漏检测

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

kmemleak 类似用户空间的 valgrind——跟踪所有内核内存分配，定期扫描内存查找**没有任何指针指向的已分配块**（潜在的泄漏）。

## 启用 kmemleak

```bash
# 内核配置
CONFIG_DEBUG_KMEMLEAK=y

# boot 参数控制扫描间隔
# kmemleak=off      — 启动时关闭
# kmemleak=on       — 启动时开启
# 默认每 10 分钟扫描一次

# 查看状态
cat /sys/kernel/debug/kmemleak
```

## 使用方法

```bash
# 1. 手动触发扫描
echo scan > /sys/kernel/debug/kmemleak
# 等待几秒让扫描完成

# 2. 查看泄漏报告
cat /sys/kernel/debug/kmemleak
# 输出示例:
# unreferenced object 0xffff000012345678 (size 128):
#   comm "my_module", pid 1234, jiffies 4294937000 (age 3600.000s)
#   hex dump (first 32 bytes):
#     00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
#   backtrace:
#     [<ffff800012345000>] kmalloc_trace+0x28/0x40
#     [<ffff800012346000>] my_init+0x48/0x200 [my_module]
#     [<ffff800012347000>] do_one_initcall+0x60/0x2e0

# 3. 清除已确认的泄漏（从报告中移除，不释放内存）
echo clear > /sys/kernel/debug/kmemleak

# 4. 再次扫描确认是否真的泄漏
echo scan > /sys/kernel/debug/kmemleak
sleep 5
cat /sys/kernel/debug/kmemleak
# 如果再次出现 → 真实泄漏（持续分配不释放）
# 如果不再出现 → 一次性分配（不是泄漏）
```

## kmemleak 原理

```
1. 跟踪所有 kmalloc/vmalloc/alloc_pages 调用
   - 记录地址、大小、分配栈
   - 维护已分配内存的元数据列表

2. 定期扫描内存
   - 扫描所有全局变量、栈、寄存器中的指针值
   - 如果某个已分配块的地址在扫描中被引用 → 不是泄漏
   - 如果没有任何指针指向它 → 潜在泄漏

3. 扫描范围
   - 全局数据段 (.data, .bss)
   - 所有任务的内核栈
   - 所有 CPU 的寄存器（通过 IPI）
   - 已分配的内存块内部（指针可能指向其他块）

4. 误报处理
   - 指针可能被混淆（如 XOR 加密）
   - 指针可能存在但 kmemleak 未扫描到
   - 用 kmemleak_not_leak() 手动标记非泄漏
   - 用 kmemleak_ignore() 忽略特定地址
```

## API

```c
#include <linux/kmemleak.h>

// 标记为非泄漏（误报）
kmemleak_not_leak(ptr);

// 忽略该地址（不追踪）
kmemleak_ignore(ptr);

// 手动注册分配（非标准分配器）
kmemleak_alloc(ptr, size, min_count, gfp);

// 手动注销释放
kmemleak_free(ptr);

// 标记为非泄漏，但继续追踪
kmemleak_annotate(ptr);

// 告诉 kmemleak 扫描这个区域内的指针
kmemleak_scan_area(ptr, size, gfp);
```

## 常见误报场景

```c
// 1. 指针被修改/加密
ptr = xor_encrypt(real_ptr, key);
// kmemleak 扫描不到原始指针 → 误报

// 2. 指针存储在设备寄存器映射的内存中
ioremap_ptr = ioremap(PCI_BAR, size);
// ioremap 的内存不在 kmemleak 扫描范围

// 3. 通过 base + offset 计算访问
char *base = kmalloc(1024, GFP_KERNEL);
// 只存储 base+offset 的差值，不存储 base 本身
u32 offset = some_value;
base[offset] = 'x';
// kmemleak 扫描不到 base 指针

// 处理方法:
kmemleak_not_leak(base);  // 标记为非泄漏
```

## HFT 关联

HFT 内核模块长时间运行（7×24），内存泄漏会导致 OOM：

1. **开发期**：定期 kmemleak 扫描，确保无泄漏
2. **测试期**：压力测试后扫描，检查长跑稳定性
3. **生产期**：非交易时段定时扫描（不实时，扫描时影响延迟）
4. **误报处理**：对已知的非泄漏调用 `kmemleak_not_leak()`

```bash
# HFT 生产环境 kmemleak 定时扫描
# crontab: 每天凌晨 3 点扫描（非交易时段）
0 3 * * * echo scan > /sys/kernel/debug/kmemleak && sleep 10 && \
    cat /sys/kernel/debug/kmemleak >> /var/log/kmemleak_$(date +\%Y\%m\%d).log

# 开发期手动扫描
echo scan > /sys/kernel/debug/kmemleak
sleep 5
cat /sys/kernel/debug/kmemleak
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kmemleak 如何判断内存是泄漏的？

> kmemleak 维护所有分配的列表。扫描时遍历所有可达内存（全局变量、栈、寄存器），收集所有看起来像指针的值。如果某分配块的地址未被任何指针引用，则判定为泄漏。原理类似垃圾回收的可达性分析。

**Q2:** kmemleak 的误报为什么常见？如何处理？

> 如果指针被修改（如 XOR 加密指针）、存储在 kmemleak 不扫描的区域（如设备寄存器映射的内存）、或通过非标准方式引用（如 base+offset 计算），kmemleak 会误判为泄漏。处理方法：在代码中调用 `kmemleak_not_leak(ptr)` 显式标记，或 `kmemleak_ignore(ptr)` 忽略。

**Q3:** kmemleak 的扫描机制是什么？

> kmemleak 维护所有已分配内存的元数据（地址/大小/调用栈）。周期性扫描内存（包括栈、全局数据、页表），寻找指向已分配块的指针。如果没有任何指针引用某个分配块，判定为泄漏。类似 GC 的 mark-sweep 但只标记不回收。

**Q4:** 为什么 kmemleak 不适合 HFT 运行时使用？

> kmemleak 扫描时需要遍历所有内存查找指针引用，扫描期间可能暂停内存分配（RCU 停顿），导致延迟毛刺。建议在非交易时段（如收盘后）手动触发扫描，而非默认的每 10 分钟自动扫描。

**Q5:** `echo clear` 和 `kmemleak_free()` 有什么区别？

> `echo clear` 是用户空间命令，从 kmemleak 报告中移除当前所有泄漏记录（但不释放内存）。用于清除已确认的"已知分配"，之后再次扫描只显示新增的泄漏。`kmemleak_free()` 是代码中的 API，告诉 kmemleak 某块内存已被释放，应停止追踪。

</details>

## 交叉引用

- [05.6 ch05 内存错误类型](chapter-05-memory-debug-1/notes/01-memory-error-types.md)
- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch06 KFENCE](chapter-06-memory-debug-2/notes/01-kfence.md)
