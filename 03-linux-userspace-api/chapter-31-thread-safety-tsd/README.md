# TLPI 第 31 章 — Thread Safety and Per-Thread Storage

**优先级**：🔴（并发库函数、无锁线程私有上下文）  
**前置**：[Ch30 同步](../chapter-30-thread-synchronization/notes.md)  
**后置**：[Ch32 线程取消](../chapter-32-thread-cancellation/notes.md)

---

## 小节目录

- [31.1 线程安全与可重入](./notes/31.1-thread-security.md)
- [31.2 `pthread_once`](./notes/31.2-pthreadonce.md)
- [31.3 TSD：`pthread_key_*`（核心）](./notes/31.3-tsd-pthreadkey.md)
- [31.4 静态 TLS：`__thread` / `_Thread_local`](./notes/31.4-thread-threadlocal.md)
- [31.5 易错 / 原理](./notes/31.5-principle.md)

---

## 章节目标


分清线程安全 / 可重入；`pthread_once`；TSD 四 API 与 strerror 改造范式；对比 `__thread` TLS；关联 C++/`errno`/嵌入式与低延迟选型。

---


---

## 与 C++ / 嵌入式 / HFT


| 栈 | |
|----|--|
| C++ | `thread_local` ≈ TLS；可直接调 pthread；**勿混**两套线程生命周期模型 |
| 用户态 only | 内核驱动无 POSIX TSD |
| HFT | `__thread` 放连接/缓冲，少锁、少抖动；禁 `strtok`/`localtime` 一类 |

---


---

## 实验清单


1. TSD 版「线程安全缓冲」  
2. `__thread` 对比  
3. （选）静态缓冲多线程错乱复现  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 可重入 ⊂ 更强；线程安全可用锁/TLS |
| 2 | 用 `_r` 或自带缓冲，别用静态内部状态 API |
| 3 | `pthread_once` + `key_create` 改造老接口 |
| 4 | key 全局、值每线程；退出跑 destructor |
| 5 | `key_delete` 不析构各线程数据 |
| 6 | 低延迟优先 `__thread` / `thread_local` |

---


---

## 参考


- Kerrisk · TLPI Ch31  
- `man 3 pthread_once` · `man 3 pthread_key_create` · `man 3 pthread_getspecific`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
