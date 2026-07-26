# 18.13 总结

> 章级精读：[../study.md#ch18-exam](../study.md#ch18-exam)

## 本节核心目标

全书安全篇收束与架构师视角。

---

## 组合防御

- 无单一万能协议：**IPsec 隧道** + **TLS 会话** + **DNSSEC 基础设施**

---

## 易混

| 问题 | 要点 |
|------|------|
| TLS vs IPsec | 应用感知 vs 对应用透明 |
| AH vs ESP | AH 不过 NAT；ESP 可加密 |
| PFS | 需 **(EC)DHE**，非静态 RSA |
| DNSSEC | 防篡改，不加密查询 |

---

## Go / Rust 实战

| 要点 | 说明 |
|------|------|
| **Go** | `crypto/tls`、`x509`；默认倾向现代 TLS |
| **Rust** | **`rustls`** + `webpki-roots` |
| **PFS** | 强制 **ECDHE** 套件，禁用老旧 RSA 密钥交换 |
| **纪律** | **永远不要自研密码算法** |

---

## 全书收束

- 传输层精读：[ch10–ch17](../../chapter09-broadcast-multicast/) · 架构：[ch01](../../chapter01-overview/study.md)
