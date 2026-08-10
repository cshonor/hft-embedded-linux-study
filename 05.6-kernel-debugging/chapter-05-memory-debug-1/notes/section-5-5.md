# 5.5 kmemleak：内核内存泄漏检测

> 🔴 精读

## 本节要点

### kmemleak 工作原理

kmemleak 类似用户空间的 valgrind——跟踪所有内核内存分配，定期扫描内存查找**没有任何指针指向的已分配块**（潜在的泄漏）。

### 启用 kmemleak

```bash
# 内核配置
CONFIG_DEBUG_KMEMLEAK=y

# boot 参数控制扫描间隔
# kmemleak=off      — 启动时关闭
# kmemleak=on       — 启动时开启
# 默认每 10 分钟扫描一次
```

### 使用方法

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

# 3. 清除已确认的泄漏
echo clear > /sys/kernel/debug/kmemleak

# 4. 再次扫描确认是否真的泄漏
echo scan > /sys/kernel/debug/kmemleak
sleep 5
cat /sys/kernel/debug/kmemleak
```

### kmemleak 的原理

```
1. 跟踪所有 kmalloc/vmalloc/alloc_pages 调用
   - 记录地址、大小、分配栈

2. 定期扫描内存
   - 扫描所有全局变量、栈、寄存器中的指针值
   - 如果某个已分配块的地址在扫描中被引用 → 不是泄漏
   - 如果没有任何指针指向它 → 潜在泄漏

3. 误报处理
   - 指针可能被混淆（如 XOR 加密）
   - 指针可能存在但 kmemleak 未扫描到
   - 用 kmemleak_not_leak() 手动标记非泄漏
   - 用 kmemleak_ignore() 忽略特定地址
```

### API

```c
// 在代码中标记
kmemleak_not_leak(ptr);    // 标记为非泄漏（误报）
kmemleak_ignore(ptr);      // 忽略该地址
kmemleak_alloc(ptr, size, min_count, gfp);  // 手动注册
kmemleak_free(ptr);        // 手动注销
```

### HFT 关联

HFT 内核模块长时间运行（7×24），内存泄漏会导致 OOM。kmemleak 可在开发期定期扫描，确保无泄漏。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kmemleak 如何判断内存是泄漏的？

> kmemleak 维护所有分配的列表。扫描时遍历所有可达内存（全局变量、栈、寄存器），收集所有看起来像指针的值。如果某分配块的地址未被任何指针引用，则判定为泄漏。原理类似垃圾回收的可达性分析。

**Q2:** kmemleak 的误报为什么常见？如何处理？

> 如果指针被修改（如 XOR 加密指针）、存储在 kmemleak 不扫描的区域（如设备寄存器映射的内存）、或通过非标准方式引用（如 base+offset 计算），kmemleak 会误判为泄漏。处理方法：在代码中调用 `kmemleak_not_leak(ptr)` 显式标记，或 `kmemleak_ignore(ptr)` 忽略。

</details>
