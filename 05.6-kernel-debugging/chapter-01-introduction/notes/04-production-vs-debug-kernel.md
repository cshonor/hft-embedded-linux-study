# 1.4 生产内核 vs 调试内核

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

不同环境需要不同的内核配置——开发环境最大化调试能力，生产环境在性能和安全之间平衡。

## 内核类型对比

| 内核类型 | 特点 | 用途 | 开销 |
|---------|------|------|------|
| **生产内核** | 无重型调试选项、高性能 | 生产环境运行 | 基准 |
| **调试内核** | 启用 KASAN/LOCKDEP/DEBUG_INFO | 开发和测试 | 2-3x |
| **混合内核** | KFENCE 等低开销选项 | 生产环境长期开启 | ~1-5% |

## 调试选项开销详细对比

| 配置项 | 功能 | 性能开销 | 内存开销 | 可生产使用 |
|--------|------|---------|---------|-----------|
| `CONFIG_DEBUG_INFO` | DWARF 调试符号 | 0% | vmlinux 变大 | ✅ |
| `CONFIG_KASAN` | 地址消毒器 | 50-100% | 1/8 内存 | ❌ |
| `CONFIG_KCSAN` | 数据竞争检测 | 10-20% | 少量 | ❌ |
| `CONFIG_LOCKDEP` | 锁依赖检测 | 5-10% | 少量 | ⚠️ 可选 |
| `CONFIG_KFENCE` | 轻量内存检测 | ~1% | ~2MB | ✅ |
| `CONFIG_UBSAN` | 未定义行为检测 | 2-5% | 少量 | ⚠️ 可选 |
| `CONFIG_HARDENED_USERCOPY` | 用户拷贝加固 | <1% | 无 | ✅ |
| `CONFIG_RANDOMIZE_BASE` | KASLR | 0% | 无 | ✅ |
| `CONFIG_STACKPROTECTOR_STRONG` | 栈溢出检测 | <1% | 少量 | ✅ |
| `CONFIG_DEBUG_LIST` | 链表操作检查 | <1% | 少量 | ✅ |
| `CONFIG_DEBUG_SG` | 散列表检查 | <1% | 少量 | ✅ |

## 生产环境推荐配置

```bash
# 生产环境可安全开启的低开销调试选项
scripts/config --enable KFENCE              # 轻量级内存检测
scripts/config --enable HARDENED_USERCOPY   # 用户空间拷贝加固
scripts/config --enable RANDOMIZE_BASE      # KASLR 地址随机化
scripts/config --enable STACKPROTECTOR_STRONG  # 栈溢出保护
scripts/config --enable DEBUG_LIST          # 链表操作检查
scripts/config --enable DEBUG_SG            # 散列表检查
scripts/config --enable HARDENED_USERCOPY_FALLBACK
scripts/config --enable REFCOUNT_FULL       # 引用计数溢出检查

# 确保 panic_on_oops 开启（崩溃后自动重启 + kdump）
scripts/config --enable PANIC_ON_OOPS
scripts/config --enable PANIC_ON_OOPS_VALUE=1

# kdump 配置
scripts/config --enable CRASH_DUMP
scripts/config --enable KEXEC
```

## 开发环境推荐配置

```bash
# 开发环境：开启所有调试选项
scripts/config --enable DEBUG_INFO
scripts/config --enable DEBUG_INFO_DWARF5
scripts/config --enable GDB_SCRIPTS
scripts/config --enable LOCKDEP
scripts/config --enable KASAN
scripts/config --enable KASAN_GENERIC
scripts/config --enable KCSAN
scripts/config --enable UBSAN
scripts/config --enable KMEMLEAK
scripts/config --enable FTRACE
scripts/config --enable FUNCTION_TRACER
scripts/config --enable FUNCTION_GRAPH_TRACER
scripts/config --enable TRACE_IRQFLAGS
scripts/config --enable DEBUG_SPINLOCK
scripts/config --enable DEBUG_MUTEXES
scripts/config --enable DEBUG_ATOMIC_SLEEP  # 检测原子上下文睡眠
scripts/config --enable WERROR              # 警告视为错误
```

## KASAN vs KFENCE：为什么 KFENCE 适合生产

| 维度 | KASAN | KFENCE |
|------|-------|--------|
| 检测方式 | 每次内存访问检查 | 概率采样（默认每 100KB） |
| 性能开销 | 50-100% | ~1% |
| 内存开销 | 1/8 物理内存 | ~2MB 固定 |
| 检测精度 | 高（每次访问） | 概率性（采样到的分配） |
| 检测类型 | OOB/UAF/双重释放 | OOB/UAF/双重释放 |
| 生产可用 | ❌ | ✅ |

```c
// KFENCE 工作原理
// 1. 每 100KB 的 slab 分配中，采样一个分配
// 2. 被采样的分配放置在 KFENCE 池中
// 3. 分配前后放置 guard page（红区）
// 4. 访问 guard page 触发页错误 → 检测到越界
// 5. 释放后页被 unmap → 后续访问触发 UAF

// 查看检测结果
# cat /sys/kernel/debug/kfence/stats
```

## HFT 关联

HFT 生产环境内核配置策略：

1. **必须开启**：KFENCE、HARDENED_USERCOPY、STACKPROTECTOR_STRONG、PANIC_ON_OOPS、kdump
2. **建议开启**：LOCKDEP（5% 开销可接受）、DEBUG_LIST/DEBUG_SG（<1%）
3. **仅开发环境**：KASAN、KCSAN、UBSAN
4. **交易热路径**：关闭所有 printk 控制台输出（`echo 1 > /proc/sys/kernel/printk`）

KFENCE 是 HFT 生产环境的"保险丝"——以 1% 的开销换取对内存错误的检测能力。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 KASAN 不适合生产环境，而 KFENCE 适合？

> KASAN 为每个内存分配添加红区 (redzone) 并在每次访问时检查，开销约 50-100%，不适合生产。KFENCE 采用概率采样（默认每 100KB 采样一次），只对被采样的分配做严格检查，开销约 1%。KFENCE 以概率方式发现内存错误，是开发和生产之间的折中。

**Q2:** 如何在不重启的情况下切换 debug 内核和生产内核？

> 用 kexec：预先加载 debug 内核到内存，生产内核崩溃时 kexec 跳转到 debug 内核分析。或用 Ksplice/Kpatch 做热补丁切换调试逻辑。但最简单可靠的方法仍然是重启切换内核 + kdump 保留崩溃现场。

**Q3:** LOCKDEP 在生产环境开启是否合理？

> 取决于系统对性能的敏感度。LOCKDEP 开销约 5-10%，对一般服务器可接受，对 HFT 热路径有影响。建议：初次部署时开启 LOCKDEP 跑一段时间，确认无锁问题后关闭。或只在测试环境开启。

**Q4:** PANIC_ON_OOPS 在生产环境应该开启吗？

> 通常应该开启。Oops 意味着内核已经处于不确定状态，继续运行可能导致数据损坏。PANIC_ON_OOPS 配合 kdump 可以自动保存崩溃现场并重启恢复。但某些高可用系统可能选择尝试继续运行（忽略非关键路径的 Oops）。

**Q5:** CONFIG_DEBUG_ATOMIC_SLEEP 检测什么？为什么重要？

> 检测在原子上下文（持有自旋锁/RCU 读锁/中断禁用）中调用可能睡眠的函数（如 mutex_lock、kmalloc(GFP_KERNEL)、copy_from_user）。这种 bug 在开发环境可能不触发，但在生产环境负载高时可能导致死锁或数据损坏。

</details>

## 交叉引用

- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch06 KFENCE](../../chapter-06-memory-debug-2/notes/01-kfence.md)
- [05.6 ch10 kdump](../../chapter-10-panic-lockup/notes/07-kdump-kexec.md)
