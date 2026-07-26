# 18.10 DNS 安全 (DNSSEC)

> 章级精读：[../study.md#ch18-9](../study.md#ch18-9) · DNS 基础：[ch11](../../chapter11-dns-domain-resolve/study.md)

## 本节核心目标

用 **DNSSEC** 防 DNS 欺骗/缓存污染。

---

## 机制

- 对 DNS 记录做**数字签名**（**RRSIG / DNSKEY / DS** 链）
- 解析方验证签名 → 确保 IP **未被篡改**

---

## 注意

- DNSSEC 保证**真实性**，**不**加密查询内容（机密性靠 **DoT/DoH**）

---

## 与第 11 章

- 弥补 DNS 明文可信缺陷 → [ch11 攻击面](../../chapter11-dns-domain-resolve/study.md)
