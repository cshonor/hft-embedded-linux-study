# 6.1 KFENCE：轻量级内存错误检测

> ⬜ 跳读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

KFENCE (Kernel Electric Fence) 是 5.x 引入的**低开销**内存错误检测器，通过**采样分配** + **页级保护**检测越界和 UAF。适合生产环境长期运行。

## 与 KASAN 的区别

| 特性 | KASAN | KFENCE |
|------|-------|--------|
| 检测方式 | 影子内存，每次访问检查 | 页保护，访问时触发 page fault |
| 覆盖率 | 100% (所有分配) | ~1% (采样) |
| 性能开销 | 2-3x slowdown | ~1% |
| 内存开销 | 1/8 用于影子 | 每个采样对象独占页 (~4KB) |
| 适用 | 开发 | 生产 |
| 检测类型 | OOB/UAF/wild/stack/global | OOB/UAF/double-free |
| 实时性 | 即时 | 即时（仅采样对象） |

## 工作原理

```
KFENCE 为采样的分配单独分配一个页:
┌─────────────────────────────────┐
│  Guard Page (不可访问)           │  ← 检测下越界
├─────────────────────────────────┤
│  Object Page                    │
│  ┌──────────┬──────────────┐   │
│  │ unused   │ allocated obj│   │  ← 对象放在页末尾
│  └──────────┴──────────────┘   │
├─────────────────────────────────┤
│  Guard Page (不可访问)           │  ← 检测上越界/UAF
└─────────────────────────────────┘

越界访问 → 触发 page fault → KFENCE 报告
UAF → 释放后页标记为不可访问 → 访问触发 fault
```

### 对象放置策略

```
对象放在页末尾的优势:
┌──────────────────────────┐
│  Page                    │
│  ┌─────────┬─────────┐   │
│  │ padding │  obj    │   │
│  └─────────┴─────────┘   │
│  ↑          ↑            │
│  下越界     上越界        │
│  → guard   → guard page  │
│  page (下)  (上)         │
└──────────────────────────┘

如果对象放在页开头:
  - 下越界 → guard page (下) ✅
  - 上越界 → 同一页的 padding，不触发 fault ❌

所以对象放在页末尾:
  - 下越界 → padding → guard page (下) ✅
  - 上越界 → guard page (上) ✅
```

## 使用方法

```bash
# 内核配置
CONFIG_KFENCE=y

# boot 参数
# kfence.sample_interval=100   — 每 100ms 采样一次 (默认)
# kfence.sample_interval=0     — 禁用
# kfence.sample_interval=10    — 更频繁采样 (更多检测，更多开销)

# 运行时调整
echo 100 > /sys/module/kfence/parameters/sample_interval  # 100ms
echo 0 > /sys/module/kfence/parameters/sample_interval    # 禁用

# 查看报告
cat /sys/kernel/debug/kfence/stats
# enabled: 1
# currently allocated: 3
# total allocations: 1234
# bugs detected: 2

# 查看详细错误
dmesg | grep -A 20 "KFENCE"
```

## KFENCE 报告示例

```
[   12.345678] ==================================================================
[   12.345680] BUG: KFENCE: out-of-bounds write in my_driver_write+0x3c/0x100
[   12.345682] Out-of-bounds write at 0xffff000012345678 (1 byte right of kfence-#42):
[   12.345690]  my_driver_write+0x3c/0x100
[   12.345695]  vfs_write+0xf4/0x2b0
[   12.345700]  ksys_write+0x74/0x100

[   12.345710] kfence-#42: 0xffff000012345677-0xffff000012345696, size=32, cache=kmalloc-32
[   12.345715] allocated by task 1234 on cpu 2 at 12.345000s:
[   12.345720]  kmalloc_trace+0x28/0x40
[   12.345725]  my_driver_init+0x48/0x200
```

## KFENCE 检测的错误类型

| 类型 | 说明 | 检测原理 |
|------|------|---------|
| out-of-bounds write | 越界写 | 写 guard page 触发 fault |
| out-of-bounds read | 越界读 | 读 guard page 触发 fault |
| use-after-free | 释放后访问 | 释放后页标记为不可访问 |
| use-after-free read | 释放后读取 | 同上 |
| double free | 重复释放 | 检测到已释放的对象再次释放 |
| invalid free | 无效释放 | 释放非 KFENCE 保护的地址 |

## HFT 关联

KFENCE 是 HFT 生产环境的"保险丝"——以 ~1% 的开销换取对内存错误的检测能力：

1. **生产可开**：1% 开销对 HFT 可接受
2. **采样检测**：概率性发现，不是 100% 覆盖
3. **配合 kdump**：检测到错误后自动 panic + kdump
4. **配合 KASAN**：开发用 KASAN，生产用 KFENCE

```bash
# HFT 生产环境 KFENCE 配置
# 内核命令行: kfence.sample_interval=1000  # 1秒采样一次（更低开销）
# 或运行时调整:
echo 1000 > /sys/module/kfence/parameters/sample_interval

# 监控 KFENCE 告警
while true; do
    bugs=$(grep "bugs detected" /sys/kernel/debug/kfence/stats | awk '{print $3}')
    if [ "$bugs" -gt 0 ]; then
        echo "ALERT: KFENCE detected $bugs bugs"
        dmesg | grep -A 20 "KFENCE"
    fi
    sleep 300
done
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KFENCE 为什么开销只有 ~1%？

> KFENCE 只对采样的分配（默认约每 100ms 一个）做页级保护，其余分配走正常 SLUB 路径。采样的分配每个独占一个 4KB 页（有 guard page），但采样率低所以总开销约 1%。KASAN 对所有分配做影子内存检查，所以开销大。

**Q2:** KFENCE 检测越界的原理是什么？

> KFENCE 将采样对象放在一个页的末尾，相邻页标记为不可访问（guard page）。如果代码越界访问对象后的字节，会触达 guard page 导致 page fault。KFENCE 在 fault handler 中报告越界。对象释放后整个页标记为不可访问，后续访问触发 UAF 报告。

**Q3:** KFENCE 和 KASAN 的主要区别是什么？

> KASAN 检查每次内存访问（通过影子内存），开销大（2-3x slowdown）但检测全面。KFENCE 只检查 slab 分配/释放，用 guard page 保护每个对象，开销极小（<1% CPU）但只检测越界和 UAF。KFENCE 适合生产环境持续运行，KASAN 适合开发环境全面检测。

**Q4:** KFENCE 的 guard page 如何工作？

> KFENCE 为每个受保护的 slab 对象分配一个独立的页，前后各加 guard page（PROT_NONE）。任何越界访问触发 page fault，KFENCE handler 报告越界。由于每个对象占一整页，内存开销大，所以 KFENCE 采样保护（默认每 100ms 采样一次）。

**Q5:** KFENCE 的 sample_interval 如何影响检测能力？

> sample_interval 越小（采样越频繁），检测到内存错误的概率越高，但开销也越大。默认 100ms 适中。HFT 生产环境可以设为 1000ms（1秒）降低开销，开发环境设为 10ms 提高检测率。设为 0 禁用 KFENCE。

</details>

## 交叉引用

- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch05 内存错误类型](chapter-05-memory-debug-1/notes/01-memory-error-types.md)
- [05.6 ch06 内存调试策略](chapter-06-memory-debug-2/notes/02-memory-debug-strategy.md)
