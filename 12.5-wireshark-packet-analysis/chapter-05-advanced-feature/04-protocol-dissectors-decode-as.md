# 5.4 协议解析

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：Dissector 如何识别协议；端口误判时用 **Decode As** 人工纠正。

## 核心知识点

### 协议解析器（Protocol Dissectors）

| 项 | 说明 |
|----|------|
| 角色 | Wireshark 内置「翻译器」：识别协议、解析字段、填充协议树 |
| 决策依据 | 端口号、魔数、启发式、已保存的 Decode As 规则 |

### 协议误判场景

| 情况 | 结果 |
|------|------|
| FTP 跑在 **443**（非标准端口） | 默认按 **TLS** 解析 → 载荷显示乱码 |
| 自定义协议占用 well-known 端口 | 错层 dissector |

### 强制解码（Decode As...）

| 步骤 | 操作 |
|------|------|
| 1 | 右键包 → `Decode As...` |
| 2 | 选择层（如 TCP port）→ 强制为 **FTP**（或其他） |
| 3 | 当前文件立即按新解析器显示 |

| 易错 | 说明 |
|------|------|
| **默认不永久保存** | 关文件后可能失效 |
| 点 **Save** | 写入用户配置 → **长期生效** |
| **建议** | 避免把临时实验规则 Save 到主配置，以免后续正常 HTTPS 被误解析 |

`Analyze` → `Enabled Protocols` 可开关某 dissector（高级排障）。

> **拓展**：**Lua** dissector 可解析私有协议；需放在 plugins 目录并启用 Lua（企业内网协议常用）。

## 抓包/实操记录

| 练习 | 说明 |
|------|------|
| 端口误判 | 找非标准端口服务 → Decode As 正确应用层 → 对比协议树 |
| 规则管理 | `Analyze` → `Decode As` 查看/删除已 Save 的规则 |

## 疑问与总结

- Decode As 解决的是**解析视角**，不是解密；加密流量仍需密钥或 TLS 密钥日志。
- 与 [Follow Stream](./05-follow-stream.md) 配合：先 Decode 正确再跟流。
