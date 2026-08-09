# 8.8 验证接收到的响应

---

## 核心主旨与安全漏洞

8.6 若 `recvfrom(..., NULL, NULL)` **忽略来源** → **混杂接收**风险。

---

## 案例

| 场景 | 后果 |
|------|------|
| 客户临时端口 **1500** | 任意主机向 `客户IP:1500` 发 UDP |
| `recvfrom` 不校验来源 | **假冒报文**被当作服务器回射打印 |

---

## 解决机制

```text
recvfrom(..., &from, &fromlen)   /* 保留来源地址 */
if (from.sin_addr != servaddr.sin_addr || from.sin_port != servaddr.sin_port)
    continue;   /* 丢弃，继续 recvfrom */
```

**源 IP + 源端口** 必须与 **sendto 目标** 严格一致。

---

## 易错细节

- **connect 后**（8.11）内核自动过滤非绑定源 — 更简单  
- 多服务器/多路径场景校验逻辑更复杂  

---

## 个人学习总结

（待填）
