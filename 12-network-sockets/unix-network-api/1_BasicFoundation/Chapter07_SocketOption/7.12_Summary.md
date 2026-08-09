# 7.12 小结

> [study.md](../study.md) · 下一章：[Ch 8 UDP](../Chapter08_BasicUDPSocket/study.md)

---

## 章节核心提炼

### 1. 选项 = 业务微调弹性

默认适合通用场景；高性能、异常控制、实时交互需 **setsockopt/fcntl**。

### 2. 服务器必备

**`bind` 之前**对 listenfd：

```c
setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
```

避免 **TIME_WAIT** 导致重启 **Address already in use**（Ch 2.7、5.x）。

### 3. 性能调优

| 选项 | 用途 |
|------|------|
| **SO_SNDBUF / SO_RCVBUF** | 缓冲、窗口（listen/connect **前**设） |
| **TCP_NODELAY** | 禁用 Nagle，低延迟交互 |

### 4. 关闭语义

**`SO_LINGER`** 改变 `close`：正常后台发、阻塞等 FIN、或 **RST 中止**（无 TIME_WAIT）。

### 5. 通向高性能

**`fcntl` GET→OR→SET `O_NONBLOCK`** → Ch 16。

---

## 易错清单

| 点 | 一句 |
|----|------|
| 继承 | 影响 accept 子连接 → **listen 前**设 listenfd |
| REUSEADDR | 重启监听必设 |
| LINGER=0 | RST，丢在途数据 |
| fcntl SETFL | 必须 **F_GETFL** 再 OR |
| Nagle | 与延迟 ACK 互等 200ms |

---

> 💡 **后续拓展留白**  
> - 阶段一 Ch1～7 选项默写表  

---

## 个人学习总结

（待填）
