# 8.7 与 ICMP 相关的攻击

> 章级精读：[../study.md#ch08-7](../study.md#ch08-7)

## 本节核心目标

认识 **Smurf**、**ICMP 重定向欺骗** 及防护思路。

---

## Smurf（历史放大 DDoS）

- 伪造受害者源 IP → 向**广播地址**发 Echo Request → 全网主机 Echo Reply 打向受害者。
- 缓解：**禁 directed broadcast**、入口过滤（BCP38）。

---

## 重定向欺骗

- 伪造 **ICMP Redirect** → 主机改路由表 → 流量经攻击者（MITM）。
- 现代 OS 对重定向较谨慎；仍应**过滤不可信重定向**。

---

## 通用原则

- 限制入站/出站 **ICMP 速率**；管理面与普通流量隔离。
- ping 用于诊断，不应暴露过大广播域。
