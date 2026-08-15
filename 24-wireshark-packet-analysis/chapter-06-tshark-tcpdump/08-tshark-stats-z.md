# 6.8 TShark 中的总结统计

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · GUI 对照：[第5章](../chapter-05-advanced-feature/chapter-summary.md)

**核心主旨**：`-z` 在纯 CLI 复现 Endpoints、Hierarchy、HTTP 统计与 **Follow Stream**。

## 核心知识点

### 基本用法

```bash
tshark -r packets.pcapng -q -z <统计类型>
```

| 参数 | 说明 |
|------|------|
| **`-q`** | Quiet，少打逐包行，突出统计表 |
| **`-z`** | 统计子命令；解析结束后打印汇总 |

### 常用 `-z` 命令

| 统计 | 命令 | 对应 GUI |
|------|------|----------|
| **IP 会话** | `tshark -r f.pcapng -q -z conv,ip` | Conversations |
| **TCP 会话** | `-z conv,tcp` | Conversations TCP |
| **端点** | `-z endpoints,ip` | Endpoints |
| **协议分层** | `-z io,phs` | Protocol Hierarchy |
| **HTTP 树** | `-z http,tree` / `-z http_req,tree` | HTTP 统计 |
| **Follow Stream** | `-z follow,tcp,ascii,0` | Follow TCP Stream |

### Follow Stream（极重要）

```bash
tshark -r file.pcapng -q -z follow,tcp,ascii,0
```

| 段 | 含义 |
|----|------|
| `follow` | 重组流 |
| `tcp` / `udp` | 协议 |
| `ascii` / `hex` | 输出编码 |
| `0` | **tcp.stream** 编号（与 GUI `tcp.stream eq 0` 一致） |

也可用地址对等形式（见 `tshark -h` 中 `-z follow` 说明）。

### 与显示过滤器组合

```bash
tshark -r f.pcapng -Y "http" -q -z http,tree
```

## 抓包/实操记录

| 任务 | 命令 |
|------|------|
| Top Talker | `-z conv,ip` 按 Bytes 列人工或排序导出 |
| 协议占比 | `-z io,phs` 对比 baseline |
| 导出 HTTP 对话 | `-z follow,http,ascii,0`（视版本支持） |

## 疑问与总结

- `-z` 列表随版本变化：`tshark -h | findstr /i "^-z"` 或查官方文档。
- 复杂图表仍不如 GUI；CLI 适合 **自动化报告** 与 **SSH 服务器**。
