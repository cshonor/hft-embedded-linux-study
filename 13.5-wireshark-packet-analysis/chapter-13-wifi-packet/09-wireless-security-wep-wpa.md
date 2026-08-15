# 13.9 无线网络安全

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：WEP/WPA 认证在管理帧与 **EAPOL 四次握手**中的成败表征。

## 核心知识点

### 13.9.1 成功的 WEP 认证（4 包）

| 序 | 内容 |
|----|------|
| 1 | AP → 客户端 **Challenge text** |
| 2 | 客户端用 WEP 密钥加密回传 |
| 3 | AP 验证成功，`Status: Successful` |
| 4 | 客户端 **Association** 完成入网 |

---

### 13.9.2 失败的 WEP 认证

| 表征 | 说明 |
|------|------|
| 错误密钥 | 加密 challenge 错误 |
| 响应 | `Authentication rejected because of challenge failure` |
| 结果 | 无关联，断开 |

---

### 13.9.3 成功的 WPA/WPA2（四次握手）

| 阶段 | 说明 |
|------|------|
| 前置 | Probe、**Associate** 之后 |
| 协议 | **EAPOL** 4 次握手（802.1X） |
| **Replay Counter** 配对 | 请求/响应配对：常为 **1, 1, 2, 2** 序列 |

**过滤器**：`eapol` · `wlan.fc.type_subtype == 0x0b`（Auth）等

解密：Wireshark `Preferences` → IEEE 802.11 → **Decryption keys** 填 PSK/PMK。

---

### 13.9.4 失败的 WPA 认证

| 表征 | 说明 |
|------|------|
| 错误 PSK | EAPOL 消息 2/4 验证失败 |
| 抓包 | **Replay Counter 重复重试**；多次失败后 |
| 踢出 | AP 发 **Deauthentication** 管理帧 |

**过滤器**：`wlan.fc.type_subtype == 0x0c`（Deauth）

## 抓包/实操记录

| 练习 | 操作 |
|------|------|
| 输错 WiFi 密码 | 手机连接失败同时 Monitor 抓 EAPOL |
| 对比 | 成功 1,1,2,2 vs 失败重试+Deauth |

**合规**：仅对自己的 AP/lab 测试；勿破解他人网络。

## 疑问与总结

- **WPA3** 握手不同，字段见新版 Wireshark 解析。
- 无密钥只能看 EAPOL 元数据，看不到内层 IP。
