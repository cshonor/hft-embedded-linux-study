# 3.10 小结

---

## 核心提炼

| 主题 | 要点 |
|------|------|
| 结构体 | `sockaddr_in` / `in6` / `storage` + 强转 `sockaddr` |
| 长度 | bind 传值；accept 传 `socklen_t*` 值—结果 |
| 字节序 | `htons` 族 |
| 地址 | `inet_pton` 族；勿 `inet_ntoa` 重入 |
| TCP | **`readn`/`writen`** + **EINTR** |

---

## 个人学习总结

（待填）
