# 7.11 fcntl 函数

> 非阻塞详述：Ch 16 · 信号驱动：Ch 25

---

## 核心主旨

**`fcntl`**（File Control）也作用于套接字 fd，控制最高频 I/O 特性。

```c
int fcntl(int fd, int cmd, ... /* int arg */);
```

---

## 核心 cmd

| 操作 | 命令 | 效果 |
|------|------|------|
| **非阻塞 I/O** | `F_SETFL` + **`O_NONBLOCK`** | read/write/connect/accept 不能立刻完成 → **`EWOULDBLOCK`**，不睡眠 |
| **信号驱动 I/O** | `F_SETFL` + **`O_ASYNC`** | 状态变化 → 内核向属主发 **`SIGIO`** |
| **套接字属主** | **`F_SETOWN`** | 指定接收 **SIGIO/SIGURG** 的进程/进程组 |

---

## 易错细节：改状态标志的正确姿势（必考）

### 错误（覆盖其它标志）

```c
fcntl(sockfd, F_SETFL, O_NONBLOCK);  /* 危险！清空 O_APPEND、O_ASYNC 等 */
```

### 正确（GET → OR → SET）

```c
int flags = fcntl(sockfd, F_GETFL, 0);
flags |= O_NONBLOCK;
fcntl(sockfd, F_SETFL, flags);
```

---

## 逻辑脉络

```text
Ch6 select 服务器阻塞 read → DoS
→ Ch7 fcntl 非阻塞铺垫
→ Ch16 系统使用非阻塞 + select/epoll
```

---

> 💡 **后续拓展留白**  
> - fcntl O_NONBLOCK vs ioctl FIONBIO  

---

## 个人学习总结

（待填）
