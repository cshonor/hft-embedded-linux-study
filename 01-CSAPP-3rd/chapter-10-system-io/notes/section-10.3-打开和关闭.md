## 10.3 打开和关闭

> **Ch10 §10.3** · [章导读](../README.md) · 上节 [§10.2 ←](./section-10.2-文件.md) · 下节 [§10.4 →](./section-10.4-读和写.md)

---

```c
int open(const char *pathname, int flags, mode_t mode);
int close(int fd);
```

常用 `flags`：

| 标志 | 含义 |
|------|------|
| `O_RDONLY` / `O_WRONLY` / `O_RDWR` | 访问模式 |
| `O_CREAT` | 不存在则创建 |
| `O_TRUNC` | 截断为 0 |
| `O_APPEND` | 写追加到末尾 |
| `O_NONBLOCK` | 非阻塞（网络常用，Ch11） |

- `close` 释放 fd；**fd 耗尽** `EMFILE` — 网关要调 `ulimit -n`

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§10.2 ←](./section-10.2-文件.md) · [本章导读](../README.md) · [§10.4 →](./section-10.4-读和写.md)
