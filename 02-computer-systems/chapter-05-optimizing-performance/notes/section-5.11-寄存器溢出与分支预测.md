## 5.11 限制因素（5.11.1–5.11.2）

### 5.11.1 寄存器溢出 (Register Spilling)

- 展开 × 多累加器 → 需要 **更多寄存器**
- 寄存器不够 → **spill 到栈** — 额外 load/store，CPE 反弹

**现象：** 展开从 4× 到 8× 反而变慢 — 用 `perf annotate` 看栈访问增多。

**HFT：** 极热循环控制 **活跃变量数量**；`-O3` 通常比瞎展开 smarter。

### 5.11.2 分支预测与误预测处罚

- 展开减少 **循环分支** 次数
- 但 `combine` 里若仍有 **数据相关分支**（如 `if (x>0)`）— `branch-misses` 仍致命

| 策略 | 适用 |
|------|------|
| `cmov` / 无分支代码 | 简单选择 |
| **排序数据** | 使分支可预测 |
| **查表 / 位掩码** | 替代分支 |
| `__builtin_expect` / `[[likely]]` | 提示布局 |

→ [Ch 3 cmov](../../chapter-03-machine-level-programs/notes/section-3.6-控制流.md) · [Ch 4 预测](../../chapter-04-processor-architecture/notes/section-4.5-PIPE流水线与冒险.md)

**HFT：** 消息类型 dispatch 用 **跳转表**；热路径避免 `if (unlikely_error)` 夹在大循环中间 — 错误处理拆到冷路径。

---

### 常见陷阱

1. **展开太多路反而变慢** — 4× 展开可能快，8× 反而慢——寄存器不够用，spill 到栈。用 `perf annotate` 检查是否出现栈访问（`mov %xmm0, -0x10(%rsp)` 等）。
2. **热循环里夹 `if (unlikely_error)`** — 即使错误几乎不发生，分支预测器仍可能误预测，导致 flush。把错误处理拆到冷路径（`cold` 属性 / 单独函数）。
3. **用 `__builtin_expect` 代替无分支** — `__builtin_expect` 只是提示分支布局，不消除分支。如果分支不可预测，`cmov`/查表/位掩码才是根本解法。

### 自测题

<details>
<summary>1. 寄存器溢出（spilling）是什么？怎么发现？</summary>

寄存器不够用，编译器把变量存到栈内存，每次用都要 load/store，CPE 反弹。**现象**：展开从 4× 到 8× 反而变慢。用 `perf annotate` 看汇编是否出现栈访问（`mov reg, offset(%rsp)`）。
</details>

<details>
<summary>2. 循环展开减少了分支，为什么 branch-misses 可能仍然很高？</summary>

展开减少的是**循环分支**（`i < n` 判断），但循环体内如果有**数据相关分支**（如 `if (data[i] > 0)`），这些分支仍然存在且可能不可预测。`branch-misses` 取决于所有分支的预测成功率，不只循环条件。
</details>

<details>
<summary>3. HFT 中如何处理热路径里的错误检查分支？</summary>

把错误处理**拆到冷路径**：①用 `__attribute__((cold))` 标记错误处理函数；②`if (unlikely(error)) { handle_error(); }` 中 `handle_error` 单独成函数不被内联；③甚至用无分支方式检查（位掩码累积错误标志，循环后统一处理）。避免不可预测分支夹在热循环中间。
</details>

---

← [本章导读](../README.md)
