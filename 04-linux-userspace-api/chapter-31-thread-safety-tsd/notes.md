# TLPI 第 31 章 — Thread Safety and Per-Thread Storage

> 对应目录：`chapter-31-thread-safety-tsd/`  
> 书名原文：**Threads: Thread Safety and Per-Thread Storage**  
> ⚠️ **可重入 ≠ 线程安全。** 老 API 用静态缓冲 → 竞态；用 `_r`、TSD（`pthread_key`）或 `__thread`/`thread_local`。HFT 优先静态 TLS，少一次查表。

**优先级**：🔴（并发库函数、无锁线程私有上下文）  
**前置**：[Ch30 同步](../chapter-30-thread-synchronization/notes.md)  
**后置**：[Ch32 线程取消](../chapter-32-thread-cancellation/notes.md)

---

## 章节目标

分清线程安全 / 可重入；`pthread_once`；TSD 四 API 与 strerror 改造范式；对比 `__thread` TLS；关联 C++/`errno`/嵌入式与低延迟选型。

---

## 31.1 线程安全与可重入

| | 含义 |
|--|------|
| **线程安全** | 多线程并发调用无 UB；可用锁/原子/TLS 保护共享 |
| **可重入** | 信号打断自身也安全；**不用**静态全局可变状态 → 通常也线程安全（更强） |

不安全经典：`strtok`、`localtime`、`ctime` → 用 `strtok_r`、`localtime_r` 等，状态由调用者提供。  
并发代码：**禁止**无 `_r` / 无文档保证的老接口。

---

## 31.2 `pthread_once`

```c
pthread_once_t once = PTHREAD_ONCE_INIT;   /* 必须静态初始化 */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
```

多线程下 init **只跑一次**；常与 `pthread_key_create` 搭配。  
`once_control` **不要** malloc。

---

## 31.3 TSD：`pthread_key_*`（核心）

痛点：接口不能加参数，但内部曾靠静态变量 → 用「每线程一份指针」。

```c
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);
int pthread_key_delete(pthread_key_t key);
```

| 点 | |
|----|--|
| key | 进程内全局一份 |
| 值 | **每线程**独立指针 |
| 线程退出 | value≠NULL → 调 destructor |
| `key_delete` | **不**对现有线程跑析构；易泄漏 |

范式（类 strerror）：

1. `pthread_once` → `key_create`  
2. 首次：`malloc` + `setspecific`  
3. 之后：`getspecific`  
4. 退出：destructor `free`  

Demo：[`code/strerror_tsd.c`](./code/strerror_tsd.c)

---

## 31.4 静态 TLS：`__thread` / `_Thread_local`

```c
static __thread char buf[256];   /* GCC；C11: _Thread_local */
```

| | `pthread_key` | `__thread` |
|--|---------------|------------|
| 时机 | 运行时 key | 编译期变量 |
| 开销 | 函数查表 | 通常更快（x86_64 FS/GS；ARM 也有 TLS 寄存器） |
| 场景 | 改老接口、动态数量 | 已知线程私有全局/静态；**HFT 优先** |

Demo：[`code/thread_local_buf.c`](./code/thread_local_buf.c)

---

## 31.5 易错 / 原理

1. `key_delete` 不跑析构  
2. destructor 内勿再 `setspecific` 同 key（循环析构风险）  
3. `errno`：每线程一份（glibc 用 TLS），不是普通全局 int  

---

## 与 C++ / 嵌入式 / HFT

| 栈 | |
|----|--|
| C++ | `thread_local` ≈ TLS；可直接调 pthread；**勿混**两套线程生命周期模型 |
| 用户态 only | 内核驱动无 POSIX TSD |
| HFT | `__thread` 放连接/缓冲，少锁、少抖动；禁 `strtok`/`localtime` 一类 |

---

## 实验清单

1. TSD 版「线程安全缓冲」  
2. `__thread` 对比  
3. （选）静态缓冲多线程错乱复现  

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

## 参考

- Kerrisk · TLPI Ch31  
- `man 3 pthread_once` · `man 3 pthread_key_create` · `man 3 pthread_getspecific`
