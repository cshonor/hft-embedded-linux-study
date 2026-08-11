# TLPI 第 30 章 — Threads: Thread Synchronization

**优先级**：🔴（多线程正确性核心）  
**前置**：[Ch29 线程导论](../chapter-29-threads-intro/notes.md)  
**后置**：[Ch31 线程安全 / TLS](../chapter-31-thread-safety-tsd/notes.md)

---

## 小节目录

- [30.1 互斥量 `pthread_mutex_t`](./notes/30.1-pthreadmutext.md)
- [30.2 条件变量 `pthread_cond_t`](./notes/30.2-pthreadcondt.md)
- [30.3 职责与易错](./notes/30.3-section-30-3.md)

---

## 章节目标


竞态与临界区；`pthread_mutex_*`；`pthread_cond_*` + while 谓词；生产者-消费者；死锁与职责分离。

---


---

## 实验清单


1. 无锁累加错误（Ch29 `thread_race`）  
2. mutex 修复  
3. （选）`trylock`  
4. while vs if  
5. 生产-消费  
6. （选）多锁死锁  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 临界区用 mutex |
| 2 | cond 永远配 mutex |
| 3 | `while (!pred) wait` |
| 4 | wait = 放锁睡，醒再加锁 |
| 5 | 多锁统一顺序防死锁 |
| 6 | mutex 管状态，cond 管等待 |

---


---

## 参考


- Kerrisk · TLPI Ch30  
- `man 3 pthread_mutex_lock` · `man 3 pthread_cond_wait`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
