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

### 自测题

<details>
<summary>1. 为什么系统调用要检查返回值？CSAPP 的包装函数做了什么？</summary>

系统调用可能失败（返回 -1，设 errno），不检查会导致**静默错误**——程序继续跑但行为错误。CSAPP 包装函数（如 `unix_error`）在系统调用返回 -1 时打印错误信息并 `exit(1)`，确保错误不被忽略。**HFT 注意**：生产系统不能直接 exit，需要更精细的错误恢复策略。

</details>

<details>
<summary>2. errno 是什么？它是线程安全的吗？</summary>

`errno` 是一个全局变量（实为线程局部存储 TLS），记录最近一次系统调用的错误码。**是线程安全的**——每个线程有自己的 errno 副本。但如果同一线程连续调用多个系统调用而不检查 errno，后者会覆盖前者的值。

</details>


---

← [本章导读](../README.md)
