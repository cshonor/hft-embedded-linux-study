# 11.20 作废的 IPv6 地址解析函数

---

## 历史演进备忘

IPv6 草案期曾出现、后被废弃：

| 函数 | 说明 |
|------|------|
| **`gethostbyname2`** | 在 `gethostbyname` 上增加地址族参数 |
| **`getipnodebyname` / `getipnodebyaddr`** | 动态分配、试图可重入 + IPv6，接口笨重 |

**RFC 3493** 以 **`getaddrinfo` / `getnameinfo`** 取代。

---

## 重点结论

阅读旧代码时识别这些符号；新代码勿用。

---

## 个人学习总结

（待填）
