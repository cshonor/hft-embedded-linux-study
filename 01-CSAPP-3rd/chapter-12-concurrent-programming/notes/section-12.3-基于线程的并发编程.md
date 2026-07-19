## 12.3 基于线程的并发编程

> **Ch12 §12.3** · [章导读](../README.md) · 上节 [§12.2 ←](./section-12.2-基于I-O多路复用的并发编程.md) · 下节 [§12.4 →](./section-12.4-多线程程序中的共享变量.md)

---

#### 12.3.1 线程执行模型

- **内核线程** — 同一进程内多线程 **共享** 地址空间、fd、堆
- 比进程轻：创建/切换成本低，但需 **同步**

#### 12.3.2–12.3.7 Posix 线程 API

```c
pthread_t tid;
pthread_create(&tid, NULL, thread_func, arg);
pthread_join(tid, &retval);   // 等待结束
pthread_detach(tid);            // 分离，资源由系统回收
pthread_cancel(tid);
pthread_exit(NULL);
```

| API | 要点 |
|-----|------|
| `pthread_create` | 新线程从 `thread_func(arg)` 开始 |
| `pthread_join` | 类似 `waitpid`，收 **retval** |
| `pthread_detach` | 不能再 `join`；避免僵尸线程 |
| 栈大小 / 属性 | `pthread_attr_t` |

- 主线程 `main` 也是线程；`exit` 会结束整个进程

#### 12.3.8 基于线程的并发服务器

```c
while (1) {
    connfd = accept(...);
    pthread_create(&tid, NULL, thread, &connfd);
    pthread_detach(tid);
}
```

- **每连接一线程** — 简单；连接数上千时线程爆炸
- 改进 → **线程池 / 预线程化**（12.5.5）

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§12.2 ←](./section-12.2-基于I-O多路复用的并发编程.md) · [本章导读](../README.md) · [§12.4 →](./section-12.4-多线程程序中的共享变量.md)
