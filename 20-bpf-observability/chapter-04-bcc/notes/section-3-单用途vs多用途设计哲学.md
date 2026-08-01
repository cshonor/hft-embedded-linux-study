# 3. 单用途 vs 多用途：设计哲学

### 单用途工具 (Single-Purpose Tools)

遵循 **Unix 哲学**：**做好一件事**。

| 例子 | 只做 |
|------|------|
| `opensnoop` | 追踪 `open()` / `openat()` |
| `runqlat` | 进程 **等 CPU** 延迟分布 |
| `tcpretrans` | TCP 重传事件 |

| 优点 | 说明 |
|------|------|
| **零门槛** | 默认参数 + 默认输出即可排障 |
| **易维护** | 行为固定，文档即契约 |
| **低认知负担** | [Ch 3 BCC 清单](../../chapter-03-performance-analysis/) 可直接当 runbook |

**HFT：** incident 第一轮优先 **单用途 + 直方图类**（`runqlat`、`biolatency`、`tcpretrans`），不要一上来写自定义 BCC。

### 多用途工具 (Multi-Purpose Tools)

**极高灵活性** — 同一引擎可挂不同函数、tracepoint、USDT，减少为每个目标写一个新工具。

| 权衡 | 说明 |
|------|------|
| **学习曲线** | 需理解参数语义与输出格式 |
| **回报** | 跨组件自定义追踪，少重复造轮子 |

本章重点四个多用途工具见下节。

---
