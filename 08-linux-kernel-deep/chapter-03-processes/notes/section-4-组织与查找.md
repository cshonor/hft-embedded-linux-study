## 4. 进程的组织与查找

---

### 一、等待队列（Wait Queues）

进程需等待事件时（磁盘 I/O 完成、定时器到期等）：

1. 内核将其放入 **等待队列**
2. 进程进入 **睡眠状态**（如 `TASK_INTERRUPTIBLE`）
3. 条件满足时，内核 **唤醒** 队列中的进程

这是阻塞 I/O、锁、定时器的通用模式。

→ 同步原语：[Ch 5 内核同步](../../chapter-05-kernel-synchronization.md) · 信号：[Ch 11](../../chapter-11-signals.md)

---

### 二、PID 哈希表

通过 **PID** 快速找到 `task_struct`：

内核维护 **四种哈希表**（2.6 模型）：

| 哈希表 | 键 | 用途 |
|--------|-----|------|
| PID | 进程 ID | `kill(pid)`、调试等 |
| TGID | 线程组 ID | 找整个线程组 |
| PGID | 进程组 ID | 作业控制、信号广播 |
| SID | 会话 ID | 终端会话管理 |

哈希冲突用 **链表** 解决。

---

### 三、HFT 关联

- 等锁 / 等 I/O 的 **唤醒延迟** 与等待队列路径相关  
- `TASK_UNINTERRUPTIBLE` 过多 → 常见于 I/O 瓶颈排查（配合 BPF/perf）

### 常见陷阱

1. 以为 PID 查找还是 ULK 讲的哈希表——6.x 用 `pidfd` 机制和 IDR/XArray 管理 PID 查找
2. 混淆 `find_task_by_pid()` 和 `pid_task()`——前者已不推荐，后者是现代 API
3. 以为 wait queue 只用于 `wait()` 系统调用——wait queue 是通用等待机制，中断/定时器/锁都使用

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 现代内核如何通过 PID 快速查找 `task_struct`？

<details><summary>答案</summary>

PID → `struct pid`（通过 `find_vpid()`，走 namespace 感知的 IDR/XArray 查找）→ `pid_task(pid, PIDTYPE_PID)` → `task_struct`。ULK 讲的 PID 哈希表已被 `pidhash` + IDR 取代。`pidfd_open()` 是 5.x 新增的 race-free PID 管理 API。

</details>

**Q2.** wait queue 和 completion 有什么区别？什么时候用哪个？

<details><summary>答案</summary>

wait queue：通用等待机制，支持条件等待（`wait_event()`）、多等待者、自定义条件。completion：专门用于「一次性完成通知」，`wait_for_completion()` + `complete()`，语义简单且无 spurious wakeup。驱动初始化等待硬件就绪用 completion；等待条件变量用 wait queue。

</details>

**Q3.** HFT 中为什么要避免 `find_task_by_pid()` 在热路径上调用？

<details><summary>答案</summary>

`find_task_by_pid()` 需要 RCU 读锁 + IDR 查找 + namespace 处理，开销在百纳秒级。HFT 热路径应缓存 `task_struct` 指针或 PID fd，避免重复查找。更好的做法是用 `pidfd` 在初始化时获取引用，热路径直接解引用。

</details>

</details>

---

← [3. 进程描述符](./section-3-进程描述符.md) · 下一节 [5. 进程切换](./section-5-进程切换.md)
