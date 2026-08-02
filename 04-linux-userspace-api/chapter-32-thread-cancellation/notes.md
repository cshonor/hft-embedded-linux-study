# TLPI 第 32 章 — Threads: Thread Cancellation

> 对应目录：`chapter-32-thread-cancellation/`  
> （勿用 `chapter-32-threads-cancellation` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 不一致）  
> 书名原文：**Threads: Thread Cancellation**  
> ⚠️ **默认：ENABLE + DEFERRED。** 只在取消点生效。清理用 `cleanup_push/pop`（须成对同块）。异步取消几乎禁用。`return` **不**跑 cleanup；`pthread_exit`/被取消会跑。

**优先级**：🟠（可取消工作线程；资源与锁安全）  
**前置**：[Ch29](../chapter-29-threads-intro/notes.md) · [Ch30](../chapter-30-thread-synchronization/notes.md)  
**后置**：[Ch33 线程更多细节](../chapter-33-threads-further/notes.md)

---

## 章节目标

`pthread_cancel`；取消状态/类型/取消点；cleanup 栈；延迟 vs 异步；join 识别 `PTHREAD_CANCELED`。

---

## 32.1–32.2 取消请求

```c
int pthread_cancel(pthread_t thread);  /* 异步投递请求，不 join */
```

| | |
|--|--|
| `pthread_exit` | 线程主动退 |
| `pthread_cancel` | 外部请求被动终止 |

可接合线程被取消后仍须 `pthread_join`。  
`join` 得 `res == PTHREAD_CANCELED` → 被取消。

Demo：[`code/thread_cancel.c`](./code/thread_cancel.c)

---

## 32.3 状态与类型

```c
pthread_setcancelstate(PTHREAD_CANCEL_ENABLE|DISABLE, &old);
pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED|ASYNCHRONOUS, &old);
```

| 状态 | |
|------|--|
| ENABLE（默认） | 接受取消 |
| DISABLE | 请求挂起，再 ENABLE 后处理 |

| 类型（仅 ENABLE 时） | |
|----------------------|--|
| **DEFERRED（默认）** | 到**取消点**才取消；工程首选 |
| ASYNCHRONOUS | 任意指令可被杀；持锁/半改数据 → 灾难 |

---

## 32.4 取消点

延迟模式下在标准函数内检查 pending，如：  
`read`/`write`/`poll`/`select`/`accept`/`connect`/`pthread_cond_wait`/`sleep`/`open`…

纯计算循环须手动：

```c
void pthread_testcancel(void);
```

Demo：[`code/thread_testcancel.c`](./code/thread_testcancel.c)

---

## 32.5 Cleanup handlers

LIFO 栈；释放锁/内存/fd。

```c
pthread_cleanup_push(routine, arg);
/* work that may be canceled */
pthread_cleanup_pop(execute);  /* !=0 执行；0 仅弹出 */
```

- `push`/`pop` **同一块作用域成对**（宏，勿跨函数乱拆）  
- 触发：取消点被取消、或 `pthread_exit`  
- **`return` 正常返回：不跑 cleanup**  

Demo：[`code/thread_cleanup.c`](./code/thread_cleanup.c)

---

## 32.6 生命周期（延迟）

1. A：`pthread_cancel(B)`  
2. B：ENABLE+DEFERRED → 到取消点  
3. 跑 cleanup 栈  
4. 终止，状态 `PTHREAD_CANCELED`  
5. join 回收  

---

## 32.7 易错清单

1. 异步取消 + 持锁 → 锁可能永占  
2. 无 cleanup → 泄漏  
3. `return` 不跑 cleanup  
4. push/pop 不成对  
5. 长循环无 `testcancel` → 永不响应  
6. 临界段可临时 DISABLE  

---

## 实验清单

1. cancel + `PTHREAD_CANCELED`  
2. `testcancel`  
3. （选）临时 DISABLE  
4. cleanup 释放资源  
5. return vs cancel 对 cleanup 差异  

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

## 参考

- Kerrisk · TLPI Ch32  
- `man 3 pthread_cancel` · `man 3 pthread_cleanup_push` · `man 3 pthread_testcancel`
