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

---
