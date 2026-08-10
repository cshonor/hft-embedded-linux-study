# SLAB 终被移除 (6.1+)

> **原文:** [Removing the SLAB allocator](https://lwn.net/Articles/949862/) (LWN, 2022)
> **内核版本:** 6.1 (SLAB 被移除)
> **对标旧书:** ULK3 Ch8 (SLAB 实现细节全部过时)

---

## 核心观点

2022 年，Linus Torvalds 接受了移除 SLAB 分配器的补丁。SLUB 在 2.6.23（2007 年）成为默认后，SLAB 仍作为可选项保留了 15 年，但几乎无人使用。

### 移除原因

| 原因 | 说明 |
|------|------|
| 无人维护 | SLAB 代码 15 年无活跃维护者 |
| 代码负担 | ~5000 行代码，增加编译时间和测试成本 |
| bug 无人修 | SLAB 专属 bug 报告无人处理 |
| 拖累 SLUB | 部分 API 需要同时支持 SLAB/SLUB/SLOB，增加复杂度 |
| 用户极少 | 调查显示 <0.1% 的用户选择 SLAB |

### 移除后的影响

```bash
# 6.1 之前: 三选一
# CONFIG_SLAB=y    # 旧 SLAB
# CONFIG_SLUB=y    # SLUB (默认)
# CONFIG_SLOB=y    # 嵌入式精简版

# 6.1 之后: 二选一
# CONFIG_SLUB=y    # SLUB (默认)
# CONFIG_SLOB=y    # 嵌入式精简版
# SLAB 选项已删除
```

### 移除过程中清理的代码

- `mm/slab.c`（~5000 行）整体删除
- `include/linux/slab_def.h` 删除
- `mm/slab_common.c` 中 SLAB 专属路径清理
- Kconfig 中 `SLAB` 选项删除

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 现代实现 |
|-------------------|---------|
| SLAB 实现细节 (array_cache, 3 条链表) | 代码已删除 |
| `kmem_cache_t` | `struct kmem_cache` (SLUB 实现) |
| SLAB 调试 (CONFIG_DEBUG_SLAB) | 改用 CONFIG_SLUB_DEBUG |

---

## HFT 关联

对 HFT 无直接影响——HFT 系统早已使用 SLUB。但移除 SLAB 减少了内核二进制大小，编译时间减少，间接有利于内核定制。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SLAB 被移除后，CONFIG_DEBUG_SLAB 的调试功能是否也丢失了？

> 没有。SLUB 有自己的调试框架 CONFIG_SLUB_DEBUG，支持 red zone (越界检测)、poison (释放后填充)、tracking (分配/释放调用栈追踪)。功能比 SLAB 的调试更强大且可运行时开关 (slub_debug= 参数)。

</details>
