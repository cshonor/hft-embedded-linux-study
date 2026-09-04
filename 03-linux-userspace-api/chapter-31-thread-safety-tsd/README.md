# TLPI 第 31 章 — Thread Safety and Per-Thread Storage

**优先级**：🔴（并发库函数选型、每线程上下文——HFT 一核一线程的基建）
**前置**：[Ch30 同步](../chapter-30-thread-synchronization/README.md)
**后置**：[Ch32 线程取消](../chapter-32-thread-cancellation/README.md)

---

## 小节目录

- [31.1 线程安全与可重入](notes/31.1-thread-safety-and-reentrancy-revisited.md) —— 四象限矩阵 + 不安全函数家族 + 改造四法
- [31.2 `pthread_once` 一次性初始化](notes/31.2-one-time-initialization.md) —— "恰好一次"的竞态与专用原语
- [31.3 TSD：`pthread_key_*`](notes/31.3-thread-specific-data.md) —— 全局 key + 每线程格子 + destructor 4 轮上限
- [31.4 静态 TLS：`__thread` / `_Thread_local`](notes/31.4-thread-local-storage.md) —— FS 段寻址实测 + 内核 CLONE_SETTLS 链路
- [31.5 总结：选型与铁律](notes/31.5-summary.md) —— 决策树 + 成本账本
- [31.6 练习](notes/31.6-exercises.md) —— 7 题 + errfmt 轮转缓冲综合题

---

## 章节目标

分清线程安全/可重入（锁防跨线程不防自己）；不安全函数家族与 `_r` 替换；`pthread_once` 的"恰好一次"；TSD 四 API 与 destructor 生命周期（4 轮上限、key_delete 不析构）；TLS 的 FS 段寻址、四种链接模型、与 TSD 的量化对比（0.5ns vs 2ns）；errno 作为 TLS 的活体案例。

**一句话主线**：`errno` 的线程安全 = TLS 的存在意义；TLS 靠内核 `clone(CLONE_SETTLS)` 设 FS base（`copy_thread():254` → `set_new_tls():130` → `ARCH_SET_FS:828`）。共享一切（地址空间），私有三样（栈、寄存器、TLS 块）。

---

## 速查：改造四法

| 方法 | 代价 | 适用 |
|------|------|------|
| 消灭全局状态 | — | **首选**（最好的同步是没有共享）|
| 调用者提供缓冲（`_r` 路线）| 改签名 | 接口还能改时 |
| TSD（运行时 key + destructor）| ~2 ns/访问，1024 key 上限 | 接口签名不能改的老接口改造 |
| TLS（`__thread`）| ~0.5 ns/访问，可内联 | 新代码每线程上下文（HFT 默认）|
| 全局锁 | 7~70 ns + 不可重入 | 低频遗留接口兜底 |

---

## 全章机制全景

```
用户态                                      内核态（x86-64, v6.6）
┌──────────────────────────────┐      ┌────────────────────────────────┐
│ errno → *__errno_location()  │      │ clone(CLONE_VM|CLONE_THREAD|   │
│   └─ __thread int（TLS）     │      │       ...|CLONE_SETTLS, tls)   │
│ __thread 变量                │      │  └─ kernel_clone()             │
│   └─ mov %fs:-N(%rip) 一条指令│      │     └─ copy_process()          │
│ pthread_getspecific(key)     │      │        └─ copy_thread():159    │
│   └─ struct pthread->specific │      │           :184 继承 fsbase     │
│      （二级格子数组，查表）   │      │           :254 CLONE_SETTLS    │
│ pthread_once（完成态）        │      │              → set_new_tls()   │
│   └─ 一条原子读（CAS+futex） │      │                :137 ARCH_SET_FS │
│ 析构：线程退出 → 扫格子 →    │      │  arch_prctl(ARCH_SET_FS):828   │
│   destructor×最多4轮         │      │   （preempt_disable + wrfsbase）│
└──────────────────────────────┘      └────────────────────────────────┘
   TSD：纯用户态（格子长在 TCB/struct pthread 上）
   TLS：用户态声明 + 内核设基址 + 硬件段寻址
```

---

## 实验清单

1. 静态缓冲串台复现（两线程打印同一字符串）✅ 31.1 demo1
2. `strtok` 竞态 vs `strtok_r` ✅ 31.1 demo2
3. 同线程二次加锁 = 信号自锁的等价复现 ✅ 31.1 demo3
4. 朴素 if-init 双重执行（8 线程跑 8 次 init）✅ 31.2 demo1
5. `pthread_once` 恰好一次（执行者是不确定 worker）✅ 31.2 demo2
6. TSD 每线程一块缓冲 + destructor 自动回收 ✅ 31.3 demo1/2
7. destructor 重塞值的 4 轮上限 ✅ 31.3 demo3
8. `PTHREAD_KEYS_MAX=1024` 实测 ✅ 31.3 demo4
9. TLS 地址每线程不同（含栈复用细节）✅ 31.4 demo1
10. TLS vs TSD 计时（0.51 vs 2.04 ns）✅ 31.4 demo3
11. errfmt 轮转缓冲嵌套安全 ✅ 31.6 综合题

---

## 易错清单

1. `printf("%s %s", f(), f())` 里 f 返回内部缓冲 → 两实参互相覆盖
2. 信号处理器里调用"加锁版线程安全"函数 → 同线程自锁死锁
3. 朴素 if-init 多重执行；init 里等别的线程 → 死锁
4. `once_control` malloc 出来（未必零填充）→ UB
5. key_create 不加 once → 每线程两份格子，全错位
6. 以为 `key_delete` 会析构 → 典型慢性泄漏
7. destructor 里 setspecific 重塞 → 4 轮后泄漏；跨线程通信 → 别人可能已退出
8. `__thread` 修饰局部变量 / 非常量初始化 → 编译错误
9. 以为 C 的 `__thread` 有析构 → 堆指针自己收
10. 把 `&tls_var` 传给别的线程 → 地址随线程消亡
11. dlopen 插件狂用 `__thread` → initial-exec 额度耗尽加载失败
12. 混用 `pthread_once_t` 与 C11 `once_flag` → 不同类型

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 可重入 ⊂ 线程安全；锁防跨线程不防自己 |
| 2 | 老接口换 `_r`/新 API；审计靠 grep |
| 3 | 能消灭的全局状态就消灭 |
| 4 | once 管恰好一次；完成态一条原子读 |
| 5 | key 全局唯一（once 保护）；值每线程 |
| 6 | getspecific 返回 NULL = 惰性分配判据 |
| 7 | destructor 最多 4 轮；key_delete 不析构 |
| 8 | `__thread` 只放全局/静态 + 常量初始化 |
| 9 | 大对象 TLS 放指针 + 惰性构造 |
| 10 | TLS 访问 = `%fs:偏移` 一条指令（~0.5ns）|
| 11 | TSD 访问 = 函数调用查表（~2ns，慢 4 倍）|
| 12 | errno 是 TLS：`*__errno_location()` |
| 13 | 内核设 FS base：`copy_thread():254` → `ARCH_SET_FS` |
| 14 | TLS 地址每线程不同且随线程消亡，别跨线程传 |
| 15 | dlopen 插件慎用 `__thread`（静态 TLS 额度）|

---

## 参考

- Kerrisk · TLPI Ch31
- `man 3 pthread_once` · `man 3 pthread_key_create` · `man 3 pthread_getspecific`
- Drepper, *ELF Handling For Thread-Local Storage*（TLS 四模型的权威文档）
- 内核源码（v6.6）：`arch/x86/kernel/process.c`（`copy_thread:159` / `set_new_tls:130`）、`arch/x86/kernel/process_64.c`（`ARCH_SET_FS:828`）
