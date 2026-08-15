# 6.4 控制输出

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 概念：[§1.1 转换 vs 分析](../chapter-01-network-basics/01-what-is-packet-analysis.md#tcpdump-止步在哪一层)

**核心主旨**：控制终端输出的颗粒度；TShark 可到 L7，tcpdump 主要 L3/L4。

## 核心知识点

### 解析深度（核心结论）

| 工具 | 深度 | 典型参数 |
|------|------|----------|
| **TShark** | 至 **L7**（HTTP、DNS 等） | **`-V`** 完整协议树 |
| **tcpdump** | 通常 **L3/L4** | **`-v` / `-vv` / `-vvv`** 递增头部细节 |

### 冗余级别（Verbosity）

**tcpdump**：`-v` → `-vv` → `-vvv`（越多越细，仍非应用层语义）

**实践**：默认输出找异常 → 再对**单包/小范围**加 `-V` 或 `-vvv`，避免「瀑布刷屏」。

### 十六进制 / ASCII

| 工具 | 十六进制 + ASCII | 仅十六进制 | 仅 ASCII |
|------|------------------|------------|----------|
| **TShark** | **`-x`**（常配合 `-r`） | — | — |
| **tcpdump** | **`-X`** | **`-x`** | **`-A`** |

```bash
tshark -r file.pcapng -x -c 5
tcpdump -r file.pcap -X -c 5
```

### TShark 常用输出形态

| 参数 | 效果 |
|------|------|
| 默认 | 单行摘要（类似列表视图） |
| `-V` | 多行协议树（量大） |
| `-T fields -e ...` | 自定义字段列（脚本友好，拓展） |

## 抓包/实操记录

| 步骤 | 命令 |
|------|------|
| 浏览 | `tshark -r a.pcapng -c 20` |
| 深挖一包 | `tshark -r a.pcapng -Y "frame.number==100" -V` |
| 对比 | 同一包 tcpdump `-vv` vs tshark `-V` |

## 疑问与总结

- CLI 优势是**管道**：`tshark ... \| grep`、`-T fields` 导出 CSV。
- 超大文件：用 [过滤器 §6.6](./06-filters.md) 瘦身，勿直接 `-V` 全文件。
