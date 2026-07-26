# 3.8 sock_ntop 和相关函数

---

## 核心主旨

`inet_ntop` 仍要传 `family`。Stevens **`sock_ntop(sockaddr*, len)`** 内查 **`sa_family`** 自动区分 v4/v6，输出 `"ip:port"`，减少 `switch(family)`。

---

## 易错细节

早期实现若用静态缓冲 → 并发需 TLS 或调用者缓冲。

---

## 个人学习总结

（待填）
