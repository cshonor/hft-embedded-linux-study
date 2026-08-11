# TLPI 第 32 章 — Threads: Thread Cancellation

**优先级**：🟠（可取消工作线程；资源与锁安全）  
**前置**：[Ch29](../chapter-29-threads-intro/notes.md) · [Ch30](../chapter-30-thread-synchronization/notes.md)  
**后置**：[Ch33 线程更多细节](../chapter-33-threads-further/notes.md)

---

## 小节目录

- [32.1 –32.2 取消请求](./notes/32.1-section-32-1.md)
- [32.3 状态与类型](./notes/32.3-state-types.md)
- [32.4 取消点](./notes/32.4-section-32-4.md)
- [32.5 Cleanup handlers](./notes/32.5-cleanup-handlers.md)
- [32.6 生命周期（延迟）](./notes/32.6-lifecycle.md)

---

## 章节目标


`pthread_cancel`；取消状态/类型/取消点；cleanup 栈；延迟 vs 异步；join 识别 `PTHREAD_CANCELED`。

---


---

## 32.7 易错清单


1. 异步取消 + 持锁 → 锁可能永占  
2. 无 cleanup → 泄漏  
3. `return` 不跑 cleanup  
4. push/pop 不成对  
5. 长循环无 `testcancel` → 永不响应  
6. 临界段可临时 DISABLE  

---


---

## 实验清单


1. cancel + `PTHREAD_CANCELED`  
2. `testcancel`  
3. （选）临时 DISABLE  
4. cleanup 释放资源  
5. return vs cancel 对 cleanup 差异  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | cancel 只投递请求；默认延迟到取消点 |
| 2 | 工程用 DEFERRED；慎用 ASYNC |
| 3 | 计算循环插 `testcancel` |
| 4 | cleanup LIFO；push/pop 成对 |
| 5 | return 不跑 cleanup；exit/cancel 会跑 |
| 6 | join 看 `PTHREAD_CANCELED` |

---


---

## 参考


- Kerrisk · TLPI Ch32  
- `man 3 pthread_cancel` · `man 3 pthread_cleanup_push` · `man 3 pthread_testcancel`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
