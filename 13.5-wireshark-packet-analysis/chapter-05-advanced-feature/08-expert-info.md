# 5.8 专家信息

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 速查：[cheatsheet/notes.md](../cheatsheet/notes.md)

**核心主旨**：用内置专家系统自动标注异常，在海量包中快速定位 TCP 丢包、重传、窗口问题。

## 核心知识点

### 入口

`Analyze` → `Expert Information`（Composite 综合视图）

也可在包列表 **Expert Info** 列、Details 面板底部查看单包级别提示。

### 四级严重度

| 级别 | 含义 | TCP 示例 |
|------|------|----------|
| **Chat** | 正常协议对话信息 | Window Update、Keep-Alive |
| **Note** | 值得注意的异常 | **Retransmission**、**Duplicate ACK**、零窗口探查 |
| **Warning** | 更严重的异常 | **Previous segment not captured**、**Zero Window**、**Fast Retransmission**、**Out-of-order** |
| **Error** | 格式错误 / 解析错误 | 畸形包、checksum 错误（若启用校验） |

### TCP 排障核心结论

专家系统在 **TCP 传输故障** 中最有价值：

```text
卡顿/慢 → Expert 过滤 Warning/Note → 看重传、零窗口、乱序、未捕获段
```

| 告警 | 常见因果链（简） |
|------|------------------|
| Duplicate ACK + Fast Retransmission | 丢包后快速重传 |
| Zero Window | 接收方缓冲区满，背压 |
| Previous segment not captured | 抓包点丢包或过滤导致序号空洞 |
| Out-of-order | 路径多径、重排 |

显示过滤器示例：`tcp.analysis.retransmission`、`tcp.analysis.zero_window`（见 cheatsheet）。

> **拓展**：Window Full 与 Retransmission 的触发关系见第 11 章 TCP 性能笔记（因果图）。

## 抓包/实操记录

| 步骤 | 操作 |
|------|------|
| 1 | 打开 Expert Information → 按 **Warning** 计数排序 |
| 2 | 点击条目跳转到对应包 |
| 3 | 结合 IO Graph / RTT 看异常时间段 |
| 4 | 显示过滤器 `tcp.analysis.flags` 或具体 expert 字段细化 |

## 疑问与总结

- Expert 是**启发式**，偶发误报；需结合抓包位置（是否 mirror 丢包）。
- 「Previous segment not captured」不一定是网络丢包，也可能是**嗅探器没抓到**。
