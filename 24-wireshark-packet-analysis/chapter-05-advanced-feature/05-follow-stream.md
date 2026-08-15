# 5.5 流跟踪

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：将分散在多个包中的应用层载荷重组为可读会话 transcript。

## 核心知识点

### Follow Stream

`右键包` → `Follow` → 选择流类型

| 类型 | 用途 |
|------|------|
| **TCP stream** | HTTP、FTP、SMTP 等基于 TCP 的明文（或已解密）载荷 |
| **UDP stream** | DNS 查询/响应等 |
| **SSL/TLS stream** | HTTPS 等；需在 `Preferences` → `Protocols` → `TLS` 配置 **RSA key log / 私钥** 方可解密 |
| **HTTP stream** | HTTP 重组与解压（专用处理） |

重组结果 = **packet transcript**（连续文本视图）。

### 视觉与交互

| 颜色 | 含义 |
|------|------|
| **红色** | 源 → 目的（常为连接发起方） |
| **蓝色** | 目的 → 源（返回方向） |

**导出**：流窗口可 Save as **ASCII**、**hex**、**C array** 等，便于取证与报告。

### 与显示过滤器联动

`Follow TCP Stream` 后 Wireshark 常自动应用 `tcp.stream eq N` 过滤器，仅显示该连接相关包。

## 抓包/实操记录

| 练习 | 目标 |
|------|------|
| HTTP 明文 | Follow TCP Stream → 读 GET/响应头 |
| DNS | Follow UDP Stream → 看查询名与应答 |
| 过滤 | `tcp.stream == 0` 切换不同连接 |

## 疑问与总结

- Follow Stream **重组应用层**；不替代看 TCP 序号、重传（用 [Expert Info](./08-expert-info.md)）。
- TLS 默认无法跟出明文；需密钥或 TLS key log file（运维预配置）。
