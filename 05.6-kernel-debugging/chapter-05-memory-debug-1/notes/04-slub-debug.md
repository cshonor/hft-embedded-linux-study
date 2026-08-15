# 5.4 SLUB debug：slab 分配器调试

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

SLUB debug 在 slab 对象周围插入 **redzone**（红区）和填充 **poison**（毒值），检测越界访问和 double free。

## 启用 SLUB debug

```bash
# 内核配置
CONFIG_SLUB_DEBUG=y

# 运行时控制 (boot 参数)
# slub_debug=FZP  — 全局启用所有调试
# F=Redzone, Z=Zero, P=Poison, T=Trace, A=Sanity

# 只对特定 cache 启用
# slub_debug=FZP,kmalloc-128
# slub_debug=FZP,kmalloc-256,dma-kmalloc-64

# 查看当前状态
cat /sys/kernel/slab/*/*/trace 2>/dev/null | head
cat /proc/slabinfo | head -20
```

## SLUB debug 标志

| 标志 | 名称 | 功能 | 开销 |
|------|------|------|------|
| F | Redzone/Fault | 对象前后插入红区检测越界 | 低 |
| Z | Zero | 分配时对象清零 | 极低 |
| P | Poison | 释放时填充毒值检测 UAF | 低 |
| T | Trace | 追踪每个对象的分配/释放栈 | 中 |
| A | Sanity | 完整性检查（元数据校验） | 中 |

## 检测机制

```
SLUB debug 布局 (kmalloc-128 为例):
┌─────────────────────────────────────────────────┐
│ Redzone (8B) │ Object Data (128B) │ Redzone (8B) │
│ 0x4F 0x4F... │  用户数据...        │ 0x4F 0x4F... │
└─────────────────────────────────────────────────┘

分配时:
  - Redzone 填充 0x4F (POISON_INUSE)
  - 对象数据区域清零 (如果 Z 选项)

释放时:
  - 对象数据填充 0x6B (POISON_FREE)
  - Redzone 检查是否被破坏

检查时机:
  - 分配时检查 Redzone → 检测前一次使用的越界
  - 释放时检查 Redzone → 检测当前使用的越界
  - 分配时检查数据区是否仍为 0x6B → 检测 UAF
```

## Poison 值含义

| 值 | 宏 | 含义 | 检测时机 |
|----|-----|------|---------|
| 0x4F | POISON_INUSE | 对象正在使用，redzone 标记 | 分配时填充 |
| 0x6B | POISON_FREE | 对象已释放，数据区填充 | 释放时填充 |
| 0x5A | POISON_END | 最后一个对象的标记 | 初始化时 |
| 0xCC | POISON_FREE (alt) | 某些架构使用 | 释放时填充 |

## 查看和验证

```bash
# 查看 slab 统计
cat /sys/kernel/slab/kmalloc-128/objs_per_slab
cat /sys/kernel/slab/kmalloc-128/slab_size
cat /sys/kernel/slab/kmalloc-128/alloc_calls
cat /sys/kernel/slab/kmalloc-128/free_calls

# 触发完整性检查
echo 1 > /sys/kernel/slab/kmalloc-128/validate
# 如果有错误，dmesg 会报告

# 追踪分配/释放
echo 1 > /sys/kernel/slab/kmalloc-128/trace
# 之后所有 kmalloc-128 的分配/释放都会在 dmesg 记录

# 查看所有 slab cache
cat /proc/slabinfo | sort -k3 -rn | head -20
# 按活跃对象数排序
```

## SLUB debug 报告示例

```
# 越界检测报告
[   12.345678] =============================================================================
[   12.345680] BUG kmalloc-128: Redzone overwritten
[   12.345682] INFO: 0xffff000012345688-0xffff00001234568f. First byte 0x61 instead of 0x4f
[   12.345690] INFO: Slab 0xfffffd0001234000 objects=24 used=12 fp=0xffff000012345600 flags=0x2000000000102000
[   12.345695] INFO: Object 0xffff000012345600 @offset=0 fp=0x0
[   12.345700] Object 0xffff000012345600: 6b 6b 6b 6b 6b 6b 6b 6b ...
```

## SLUB debug vs KASAN

| 维度 | SLUB debug | KASAN |
|------|-----------|-------|
| 检测时机 | 分配/释放时 | 每次访问 |
| 实时性 | 延迟（下次操作时发现） | 即时 |
| 精度 | redzone 粒度 | 字节级 |
| 开销 | 低 | 高 (2-3x) |
| 内存 | 对象+redzone | 1/8 影子 |
| 适用 | 轻量调试 | 全面调试 |

## HFT 关联

HFT 开发环境 SLUB debug 使用策略：

1. **开发期**：`slub_debug=FZP` 全局启用，配合 KASAN
2. **注意冲突**：KASAN 启用时不额外启用 SLUB redzone（F flag），但 Z（清零）和 T（追踪）仍可用
3. **特定 cache**：只对问题相关的 cache 启用，减少开销
4. **生产不用**：增加分配延迟，影响实时性

```bash
# HFT 开发环境启动参数
# slub_debug=FZP,kmalloc-128,kmalloc-256,kmalloc-512
# 只对常用大小的 kmalloc cache 启用调试
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SLUB debug 和 KASAN 都能检测越界，有什么区别？

> SLUB debug 在对象周围插入 redzone 字节，只能在分配/释放时检查 redzone 是否被破坏——是**延迟检测**（下次操作时发现）。KASAN 通过影子内存实时检查每次访问——是**即时检测**。KASAN 更精确但开销更大。SLUB debug 开销较小但检测能力有限。

**Q2:** `slub_debug=FZP` 中的 F/Z/P 分别有什么作用？

> F (Redzone/Fault): 在对象前后插入红区检测越界。Z (Zero): 分配时将对象清零。P (Poison): 释放时将对象填充毒值检测 UAF。三者组合提供基础的内存错误检测。T (Trace) 还可以追踪每个对象的分配/释放栈。

**Q3:** SLUB debug 的 redzone 如何检测越界访问？

> redzone 在每个 slab 对象前后填充特殊字节（如 0x4F）。如果代码越界写入，redzone 被破坏。下次 slab 操作（alloc/free）时检查 redzone 完整性，发现损坏报告越界。局限：只在 slab 操作时检测，不实时。KASAN 实时检测更可靠。

**Q4:** SLUB debug 和 KASAN 同时使用会冲突吗？

> 会部分冲突。两者都在对象周围加 redzone，可能重复。推荐：KASAN 启用时不额外启用 SLUB debug redzone（CONFIG_SLUB_DEBUG 的 F flag）。但 SLUB debug 的 poisoning（Z flag）和 tracking（T flag）仍可与 KASAN 配合。

**Q5:** 如何只对特定 slab cache 启用 SLUB debug？

> 使用 boot 参数 `slub_debug=FZP,kmalloc-128`，只对 kmalloc-128 cache 启用调试。或运行时通过 sysfs：`echo 1 > /sys/kernel/slab/kmalloc-128/store_user`。这样可以减少非关键 cache 的调试开销。

</details>

## 交叉引用

- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch05 内存错误类型](../../chapter-05-memory-debug-1/notes/01-memory-error-types.md)
- [05.6 ch05 kmemleak](../../chapter-05-memory-debug-1/notes/05-kmemleak.md)
