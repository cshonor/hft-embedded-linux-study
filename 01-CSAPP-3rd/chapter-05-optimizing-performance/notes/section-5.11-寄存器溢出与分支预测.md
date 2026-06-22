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

→ [Ch 3 cmov](../chapter-03-machine-level-programs/notes/section-3.6-控制流.md) · [Ch 4 预测](../chapter-04-processor-architecture/notes/section-4.5-PIPE流水线与冒险.md)

**HFT：** 消息类型 dispatch 用 **跳转表**；热路径避免 `if (unlikely_error)` 夹在大循环中间 — 错误处理拆到冷路径。

---

← [本章导读](../README.md)
