# 6.1 KFENCE：轻量级内存错误检测

> ⬜ 跳读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

### KFENCE (Kernel Electric Fence)

KFENCE 是 5.x 引入的**低开销**内存错误检测器，通过**采样分配** + **页级保护**检测越界和 UAF。

### 与 KASAN 的区别

| 特性 | KASAN | KFENCE |
|------|-------|--------|
| 检测方式 | 影子内存，每次访问检查 | 页保护，访问时触发 page fault |
| 覆盖率 | 100% (所有分配) | ~1% (采样) |
| 性能开销 | 2-3x slowdown | ~1% |
| 内存开销 | 1/8 用于影子 | 每个采样对象独占页 (~4KB) |
| 适用 | 开发 | 生产 |

### 工作原理

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

### 使用方法

```bash
# 内核配置
CONFIG_KFENCE=y

# boot 参数
# kfence.sample_interval=100   — 每 100ms 采样一次 (默认)
# kfence.sample_interval=0     — 禁用
# kfence.sample_interval=10    — 更频繁采样 (更多检测，更多开销)

# 查看报告
cat /sys/kernel/debug/kfence/stats
# enabled: 1
# currently allocated: 3
# total allocations: 1234
# bugs detected: 2

# 查看详细错误
dmesg | grep -A 20 "KFENCE"
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KFENCE 为什么开销只有 ~1%？

> KFENCE 只对采样的分配（默认约每 100ms 一个）做页级保护，其余分配走正常 SLUB 路径。采样的分配每个独占一个 4KB 页（有 guard page），但采样率低所以总开销约 1%。KASAN 对所有分配做影子内存检查，所以开销大。

**Q2:** KFENCE 检测越界的原理是什么？

> KFENCE 将采样对象放在一个页的末尾，相邻页标记为不可访问（guard page）。如果代码越界访问对象后的字节，会触达 guard page 导致 page fault。KFENCE 在 fault handler 中报告越界。对象释放后整个页标记为不可访问，后续访问触发 UAF 报告。

</details>
