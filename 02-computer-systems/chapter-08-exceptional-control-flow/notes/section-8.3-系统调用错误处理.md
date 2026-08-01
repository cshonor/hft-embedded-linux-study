## 8.3 系统调用错误处理

- 大多数 **syscall 包装函数** 失败时返回 **-1**，错误码在 **`errno`**
- 必须检查返回值；`perror` / `strerror` 打印

```c
if ((fd = open(path, O_RDONLY)) < 0) {
    perror("open");
    exit(1);
}
```

### EINTR — HFT 网络代码常见

慢 syscall（`read`/`write`/`accept`）可能被 **信号打断**，返回 `-1` 且 `errno == EINTR`：

```c
ssize_t n;
do {
    n = read(fd, buf, len);
} while (n < 0 && errno == EINTR);
```

- 非阻塞 + `epoll` 路径 EINTR 仍会出现 — **重试或交给事件循环**

**包装习惯：** 本书 `unix_error` / `app_error` 宏 — 生产用统一日志 + 指标。

---

← [本章导读](../README.md)
