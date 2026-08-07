# 5. 规范的工具文档

BCC 工具面向 **生产环境**：需 **root**，且每个工具都有标准文档。

### Man Pages（手册页）

| 内容 | 说明 |
|------|------|
| **原理** | 挂哪些 probe、内核里做什么聚合 |
| **开销估算** | 能否常驻、对延迟的大致影响 |
| **输出字段** | 每列含义 |
| **参数** | `-p` PID、`-c` 命令、`-d` 秒数、过滤表达式等 |

```bash
man funccount-bpfcc
man stackcount-bpfcc
```

### Examples Files（示例文件）

发行版通常在 `/usr/share/bcc/examples/doc/`（路径因包而异）：

| 特点 | 价值 |
|------|------|
| **真实命令 + 输出截图** | 比干读 man 更快建立直觉 |
| **逐段解读** | 对照「这一列说明什么」 |

**学习路径建议：** `man` 看参数 → `examples` 看场景 → 本机跑一遍 → 对照 [Ch 3 清单](../../chapter-03-performance-analysis/) 纳入 runbook。


### 常见陷阱

1. **忽视工具的 --help 和 man 页面** — BCC 工具有详细的 man 页面和 --help 输出，包含参数说明和示例；不看文档靠猜参数会浪费时间
2. **混淆工具的选项和示例中的占位符** — 文档示例中的函数名、PID 是占位符，需替换为实际值；直接复制示例可能因目标不存在而无输出
3. **不看工具的 EXAMPLES 部分** — BCC man 页面有 EXAMPLES 段落展示典型用法；很多工具的隐藏功能只在示例中体现

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 工具文档的标准结构包含哪些部分？**

   <details>
   <summary>参考答案</summary>

   SYNOPSIS（命令格式）、DESCRIPTION（功能描述）、OPTIONS（参数说明）、EXAMPLES（典型用法示例）、OVERHEAD（开销说明）、SOURCE（源码位置）、OS（支持的内核版本）、STABILITY（稳定性说明）、SEE ALSO（相关工具）。

   </details>

2. **如何快速了解一个不熟悉的 BCC 工具？**

   <details>
   <summary>参考答案</summary>

   (1) `tool --help` 看参数概要；(2) `man tool`（或 `man 8 tool`）看完整文档；(3) 重点看 EXAMPLES 段落的典型用法；(4) 看 OVERHEAD 了解开销是否可接受；(5) 看 SOURCE 找到源码（/usr/share/bcc/tools/）了解实现细节。

   </details>

3. **BCC 工具的 OVERHEAD 字段对 HFT 有什么意义？**

   <details>
   <summary>参考答案</summary>

   OVERHEAD 说明工具的预期开销（如「低开销，适合长期运行」或「高开销，仅短跑」）。HFT 场景特别关注：(1) 工具是否影响被测路径的延迟；(2) 是否适合在最低延迟核上运行；(3) 是否需要短跑（seconds）还是可长期挂载（minutes+）。

   </details>

</details>

---
