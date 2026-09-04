# TLPI 第 29 章 — Threads: Introduction

**优先级**：🔴（并发基础；下一章同步）
**前置**：[Ch28 fork/exec 深潜](../chapter-28-process-creation-exec-detail/README.md)
**后置**：[Ch30 线程同步](../chapter-30-thread-synchronization/README.md)

---

## 一句话内核视角

> **Linux 内核里没有「线程」对象。** `fork()`、`vfork()`、`pthread_create()`、`clone3()` 全部走同一个 `copy_process()`（`kernel/fork.c:2240`），差别只是一组 `CLONE_*` 位。
> 「进程」和「线程」是**用户态给两组不同 flag 组合起的名字**。

```
  CLONE_THREAD  ⟹  CLONE_SIGHAND  ⟹  CLONE_VM
  （同一线程组）    （共享信号处理）   （共享地址空间）
        kernel/fork.c:2267        kernel/fork.c:2275
```

---

## 小节目录

| 节 | 主题 | 核心 API | 一句话 |
|----|------|----------|--------|
| [29.1 概念总览](notes/29.1-overview.md) | 线程模型 | — | 共享 vs 私有；内核里只有 `task_struct` |
| [29.2 Pthreads 规范背景](notes/29.2-background-details-of-the-pthreads-api.md) | 标准与实现 | `-pthread` | **返回错误码，不设 `errno`** |
| [29.3 创建线程](notes/29.3-thread-creation.md) | `pthread_create` | `pthread_create` | 新线程**可能在 create 返回前就跑** |
| [29.4 终止线程](notes/29.4-thread-termination.md) | `pthread_exit` | `pthread_exit` | 线程里 **`exit()` 杀全进程**；线程不产生僵尸 |
| [29.5 线程 ID](notes/29.5-thread-ids.md) | 两套 ID | `pthread_self` / `gettid` | `getpid()` 返回 TGID；`pthread_t` 会复用 |
| [29.6 连接已终止的线程](notes/29.6-joining-with-a-terminated-thread.md) | `pthread_join` | `pthread_join` | 睡在 futex 上等，不是轮询 |
| [29.7 分离线程](notes/29.7-detaching-a-thread.md) | `pthread_detach` | `pthread_detach` | **内核里没有 detach**，只有 glibc 的 `__free_tcb()` |
| [29.8 线程属性](notes/29.8-thread-attributes.md) | `pthread_attr_t` | `pthread_attr_*` | 默认 `inheritsched=INHERIT` → 调度设置**静默失效** |
| [29.9 线程 vs 进程](notes/29.9-threads-versus-processes.md) | 选型 | `fork` vs `pthread_create` | 线程切换便宜 = 不写 CR3 + 不跑 Spectre 缓解 |
| [29.10 总结](notes/29.10-summary.md) | 全章速览 | — | 五个反直觉真相 + 十条铁律 |
| [29.11 练习](notes/29.11-exercises.md) | 7 题 + 综合题 | — | 生产级线程池 + 优雅关闭 |

---

## 章节目标

1. 建立 Pthreads 模型，能画出**共享 / 私有**资源的完整边界
2. 掌握 `create` / `join` / `detach` / `exit` 四件套，并知道它们各自的**内核机制**
3. 能解释「为什么线程切换便宜」到 `arch/x86/mm/tlb.c:562` 这个层面
4. 认识竞争条件的存在，为下一章互斥锁铺垫
5. 能在 HFT / 嵌入式两个方向上做线程模型的选型

---

## 共享 vs 私有（速查）

| 资源 | 线程间 | 决定 flag | 内核函数 |
|------|--------|-----------|----------|
| 地址空间 / 页表 / 堆 / 全局变量 | **共享** | `CLONE_VM` | `copy_mm():1708`，分支 `:1731` |
| 文件描述符表 | **共享** | `CLONE_FILES` | `copy_files():1766`，分支 `:1784` |
| cwd / umask | **共享** | `CLONE_FS` | `copy_fs():1746` |
| **信号处理器（`sigaction`）** | **共享** | `CLONE_SIGHAND` | `copy_sighand():1799`，分支 `:1803` |
| **信号掩码（`sigprocmask`）** | **私有** | — | `task_struct->blocked` |
| `errno` | **私有** | — | glibc `__errno_location()` |
| TLS（`__thread`） | **私有** | `CLONE_SETTLS` | `uapi/linux/sched.h:22` |
| 用户栈 | **私有**（默认 8MB） | — | glibc `allocate_stack()` |
| **内核栈** | **私有 16KB** | — | `kernel/fork.c:309` |
| TGID | **相同** | `CLONE_THREAD` | `:2565-2572` |
| 内核 pid（TID） | **不同** | — | — |
| `exit_signal` | **-1（不发 SIGCHLD）** | `CLONE_THREAD` | `:2632-2633` |

---

## 核心 API

```c
/* 编译链接：-pthread（不是 -lpthread） */

/* 创建 */
int pthread_create(pthread_t *t, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg);

/* 身份 */
pthread_t pthread_self(void);
int       pthread_equal(pthread_t a, pthread_t b);      /* 不用 == */
pid_t     gettid(void);                                 /* 内核 TID，诊断用 */

/* 终止 */
void pthread_exit(void *retval);

/* 回收（二选一，契约唯一） */
int pthread_join(pthread_t t, void **retval);
int pthread_detach(pthread_t t);
int pthread_timedjoin_np(pthread_t t, void **retval, const struct timespec *abstime);

/* 属性（init → set → create → destroy） */
pthread_attr_init / _destroy
pthread_attr_setdetachstate (&a, PTHREAD_CREATE_{JOINABLE,DETACHED})
pthread_attr_setstacksize   (&a, bytes)          /* ≥ PTHREAD_STACK_MIN(16384) */
pthread_attr_setguardsize   (&a, bytes)
pthread_attr_setschedpolicy (&a, SCHED_{OTHER,FIFO,RR,BATCH,IDLE,DEADLINE})
pthread_attr_setschedparam  (&a, &sched_param)
pthread_attr_setinheritsched(&a, PTHREAD_EXPLICIT_SCHED)   /* ★ 必须 */
pthread_attr_setaffinity_np (&a, sizeof(cpu_set_t), &cpuset)
```

### 返回值约定（最容易踩的一条）

| | Pthreads | 传统 libc |
|---|---|---|
| 成功 | `0` | `0` / 有效句柄 |
| 失败 | **正错误码**（`EAGAIN`/`EINVAL`/`EPERM`/`ESRCH`…） | `-1` |
| `errno` | **不设置** | 设置 |
| 判断 | `if ((rc = pthread_xxx(...)) != 0)` | `if (... < 0)` |
| 打印 | `strerror(rc)` | `perror()` |

---

## 生命周期全景（内核路径）

```
  pthread_create()
      │ clone(CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD
      │       |CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID
      │       |CLONE_CHILD_SETTID|CLONE_CHILD_CLEARTID, stack)
      ↓
  kernel_clone()                              kernel/fork.c:2868
      └─ copy_process()                              :2909 → :2240
           ├─ CLONE_THREAD⟹SIGHAND / ⟹VM 校验         :2267 / :2275
           ├─ set_child_tid / clear_child_tid         :2347 / :2351
           ├─ RLIMIT_NPROC 线程数上限                  :2366
           ├─ copy_mm/fs/files/sighand/signal/thread  :2489-2510
           ├─ exit_signal = -1（线程不发 SIGCHLD）      :2632-2633
           └─ put_user(nr, parent_tid) ← 父拿 TID      :2924
      ↓
  线程跑 fn(arg) → return / pthread_exit()
      ↓
  do_exit()                                   kernel/exit.c:809
      ├─ mm_release()                         kernel/fork.c:1616
      │    ├─ put_user(0, clear_child_tid)            :1634  ← TID 清零
      │    └─ do_futex(FUTEX_WAKE, 1)                 :1635  ← 唤醒 joiner
      └─ exit_notify()                        kernel/exit.c:727
           ├─ leader   → EXIT_ZOMBIE + SIGCHLD
           └─ 非 leader → autoreap = true  :750 → EXIT_DEAD :753-756
                          （内核里没有僵尸线程）
      ↓
  glibc 收尾
      ├─ joinable：等 pthread_join → __free_tcb() → munmap
      └─ detached：start_thread 自己 __free_tcb() → munmap
```

**进程退出时的另一条路**：`exit_group:1033` → `do_group_exit():999` → `zap_other_threads()`（`kernel/signal.c:1378`）→ 所有线程挂 SIGKILL。

---

## 五个反直觉真相

| # | 直觉 | 真相 |
|---|------|------|
| ① | 线程是内核里的一类对象 | 内核只有 `task_struct`，差别只是 `CLONE_*` |
| ② | 忘了 join 会留僵尸线程 | 内核 `autoreap` 立即回收；泄漏的是 glibc 的 8MB 栈 |
| ③ | `pthread_join` 在轮询 | 睡在 futex 上，内核清 TID 后 `FUTEX_WAKE` |
| ④ | `getpid()` 返回线程 ID | 返回 TGID，所有线程相同；用 `gettid()` |
| ⑤ | 线程切换因为「不换页表」而便宜 | 具体是 `real_prev == next` 早退：不写 CR3、**不跑 `cond_mitigation()`**（`arch/x86/mm/tlb.c:562` vs `:612`） |

---

## 易错清单（15 条）

| # | 错误 | 后果 | 正解 |
|---|------|------|------|
| 1 | 线程里调用 `exit()` | **整个进程没了** | `return` / `pthread_exit()` |
| 2 | 主线程 `main` `return` | 拖死所有线程 | 要等就 `pthread_join` / `pthread_exit` |
| 3 | `if (pthread_create(...) == -1)` | 错误码是正数，永远不触发 | `!= 0` |
| 4 | `perror("pthread_create")` | `errno` 没被设置，打印的是残留值 | `strerror(rc)` |
| 5 | `-lpthread` 代替 `-pthread` | glibc 2.34+ 后是空壳 | `-pthread` |
| 6 | 传 `&i`（循环变量地址） | 所有线程读到同一个在变的地址 | 传值 / 每线程独立对象 |
| 7 | 传即将失效的栈地址 | 未定义行为 | `static` / 堆分配 / 预分配 slot |
| 8 | 用 `==` 比较 `pthread_t` | 不可移植 | `pthread_equal()` |
| 9 | 把 `pthread_t` 当长期身份 | 终止后会被新线程复用 | `slot` 下标 + `gettid()` |
| 10 | 用 `getpid()` 打线程日志 | 所有线程 ID 一样 | `gettid()` |
| 11 | 设了调度属性却没设 `EXPLICIT_SCHED` | **静默失效**，延迟特性全错 | 三件套 + 创建后读回校验 |
| 12 | 无同步读写共享变量 | 竞争（Ch30） | 互斥锁 / 原子 / 无锁队列 |
| 13 | 栈上放 > 4KB 的对象 | 一步跨过 guard，静默踩到别的线程栈 | `malloc` / 静态池；调大 guard |
| 14 | 既 join 又 detach | `pthread_detach` 静默返回 0 却什么也没做 | 契约唯一 |
| 15 | 多线程程序里 `fork()` | 子进程只留调用线程，锁状态错乱 → 死锁 | `fork`+`exec` / `pthread_atfork` / `posix_spawn` |

---

## HFT / 嵌入式关联

| 维度 | HFT | 嵌入式 |
|------|-----|--------|
| 模型 | 同进程多线程（共享订单簿）+ 独立进程（日志/网关/风控） | 多线程为主，关键控制回路独立进程 |
| 线程数 | 少而精，一核一线程 | 10~50，受内存限制 |
| 栈 | 512KB~1MB + guard 64KB | 16~64KB（无 MMU 平台更小） |
| 调度 | `SCHED_FIFO` 分层 + `RLIMIT_RTPRIO` 上界 | `SCHED_FIFO` + 绑核 |
| 通信 | SPSC 无锁环形队列，零系统调用 | 共享内存 / 消息队列 |
| 创建时机 | **启动期全建好**，运行期零创建 | 启动期建好 |
| 回收 | 全部 joinable + `timedjoin_np` | joinable（要确认）/ detached（心跳） |
| 禁忌 | 热路径 `fork` / `printf` / `malloc` | 栈上放大对象 / 深递归 |

---

## 实验清单

| # | 实验 | 对应节 | 观察什么 |
|---|------|--------|----------|
| 1 | `share_or_not.c` | 29.1 | 全局变量共享、`errno` 私有 |
| 2 | `pthread_api.c` | 29.2 | 返回值是正错误码，不是 -1 |
| 3 | `thread_create_demo.c` | 29.3 | 传参三大陷阱 |
| 4 | `thread_exit_demo.c` | 29.4 | `exit()` 杀全进程；优雅关闭序列 |
| 5 | `thread_ids.c` | 29.5 | `getpid()` 全相同、`gettid()` 不同 |
| 6 | `pthread_join_demo.c` | 29.6 | 批量 join 总耗时 = max |
| 7 | `detach_demo.c` | 29.7 | VIRT 泄漏与补 join 回落 |
| 8 | `attr_demo.c` | 29.8 | 默认 `inheritsched=INHERIT`；栈布局实测 |
| 9 | `tp_bench.c` | 29.9 | 创建/切换开销对比；崩溃影响域 |
| 10 | `thread_pool.c` | 29.11 | 优雅关闭 + 超时 join 兜底 |

**系统侧验证**：

```bash
ps -eLf | grep <prog>                  # 每个线程一行，LWP 列 = 内核 TID
top -H -p <pid>                        # 按线程看 CPU
ls /proc/<pid>/task/                   # 每线程一个目录
grep Threads /proc/<pid>/status
cat /proc/self/statm                   # VIRT/RSS，观察线程泄漏
ulimit -r 50 && ./prog                 # 给 RT 优先级权限（RLIMIT_RTPRIO）
gcc -fstack-usage -O2 *.c && cat *.su  # 静态估算每函数栈帧
```

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 内核里没有线程对象，只有 `CLONE_*`；`CLONE_THREAD⟹SIGHAND⟹VM` |
| 2 | `-pthread` 编译；**成功返回 0，失败返回正错误码，不设 `errno`** |
| 3 | 线程里禁 `exit()`（杀全进程）；主线程 `return` 拖死所有线程 |
| 4 | `getpid()` = TGID（全线程相同）；`gettid()` = 内核 pid（每线程不同） |
| 5 | `pthread_t` = `struct pthread *`（栈地址），**会复用**；比较用 `pthread_equal()` |
| 6 | `pthread_join` 睡在 futex 上；内核 `mm_release()` 清 TID + `FUTEX_WAKE` |
| 7 | 内核 `autoreap` 立即回收线程 → **没有僵尸线程**；泄漏的是 glibc 8MB 栈 |
| 8 | **内核里没有 detach**，两态的内核路径完全相同 |
| 9 | 每个线程必须 join 或 detach，**契约唯一** |
| 10 | 默认 `inheritsched = INHERIT` → 调度设置**静默失效**，必须设 `EXPLICIT` |
| 11 | 线程切换便宜 = 不写 CR3 + 不换 ASID + **不跑 `cond_mitigation()`** |
| 12 | `fork` 开销与虚拟内存大小成正比（`dup_mm`），大内存进程绝不 fork |
| 13 | 新线程栈固定不增长；guard 只挡小步溢出，≥4KB 的单次写入会跨过去 |
| 14 | 多线程程序里 `fork()` 危险：只留调用线程，锁状态错乱 |
| 15 | 共享地址空间 ⇒ 必然有竞争 ⇒ 下一章互斥锁与条件变量 |

---

## 代码示例

入门示例见 [`code/`](./code/)（`simple_thread.c`、`detached_thread.c`、`thread_exit_retval.c`、`thread_race.c`）。

完整可运行的生产级示例在各节的「代码示例」小节：

| 示例 | 节 | 演示内容 |
|------|----|----------|
| `share_or_not.c` | 29.1 | 共享 vs 私有逐项实测 |
| `pthread_api.c` | 29.2 | 返回值约定、错误码 |
| `thread_create_demo.c` | 29.3 | 传参陷阱、栈布局 |
| `thread_exit_demo.c` | 29.4 | 五种终止方式、优雅关闭 |
| `thread_ids.c` | 29.5 | 两套 ID、`pthread_t` 复用 |
| `pthread_join_demo.c` | 29.6 | join 三重身份、批量 join |
| `detach_demo.c` | 29.7 | 三种 detach 写法、VIRT 泄漏 |
| `attr_demo.c` | 29.8 | 属性默认值、栈布局、调度权限 |
| `tp_bench.c` | 29.9 | 创建/切换开销、崩溃影响域 |
| `thread_pool.c` | 29.11 | 线程池 + 优雅关闭（综合题） |

---

## 参考

- Kerrisk · TLPI Ch29「Threads: Introduction」
- `man 7 pthreads` · `man 3 pthread_create` · `man 3 pthread_join` · `man 3 pthread_detach`
- `man 3 pthread_attr_init` · `man 3 pthread_attr_setinheritsched` · `man 3 pthread_setaffinity_np`
- `man 2 clone` · `man 2 futex` · `man 7 sched` · `man 2 sched_setattr`
- 内核源码（v6.6）：`kernel/fork.c`、`kernel/exit.c`、`kernel/sched/core.c`、`arch/x86/mm/tlb.c`

---

## 导航

- 上级：[03-linux-userspace-api](../README.md) · [OUTLINE](../OUTLINE.md)
- 上一章：[Ch28 fork/exec 深潜](../chapter-28-process-creation-exec-detail/README.md)
- 下一章：[Ch30 线程同步](../chapter-30-thread-synchronization/README.md)
- 相关：[Ch31 线程安全与 TSD](../chapter-31-thread-safety-tsd/README.md) · [Ch32 线程取消](../chapter-32-thread-cancellation/README.md) · [Ch33 线程进阶](../chapter-33-threads-further/README.md) · [Ch35 优先级与调度](../chapter-35-process-priorities-scheduling/README.md)
