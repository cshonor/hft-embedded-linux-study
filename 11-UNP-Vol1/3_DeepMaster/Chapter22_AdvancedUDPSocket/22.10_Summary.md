# 22.10 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 突破 recvfrom 信息黑洞

多宿主 UDP 服 → **`recvmsg` + IP_RECVDSTADDR / IPV6_RECVPKTINFO** 等辅助数据。

### 2. 截断

**`MSG_TRUNC`**（`recvmsg`），勿默默损坏数据。

### 3. 选型与可靠性

- 勿用 UDP 扛大块可靠业务  
- 若请求-应答：Jacobson **RTO**、**序号**、退避  

### 4. 并发窘境

无连接分路 → 优先**迭代/线程池**；fork 新端口模式罕见且客户端/NAT 苛刻。

---

## Ch 8 → Ch 22 升级路径

```text
recvfrom/sendto → recvmsg + 选项 + MSG_TRUNC
无超时 → 14章超时 + 22.5 RTO/序号
单套接字迭代 → 22.6 多 bind / 22.7 并发权衡
```

---

## 个人学习总结

（待填）
