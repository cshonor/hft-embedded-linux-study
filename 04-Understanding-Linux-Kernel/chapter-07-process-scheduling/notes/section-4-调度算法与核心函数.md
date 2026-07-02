## 4. 调度算法与核心函数

---

### 一、`scheduler_tick()` — 时钟 tick 入口

在 **时钟中断** 路径中（[Ch 6](../chapter-06-timing/)）：

1. **递减** 当前进程时间片  
2. 若耗尽 → 移出 **活动队列**  
3. 设置 **`TIF_NEED_RESCHED`**

---

### 二、`recalc_task_prio()` — 动态优先级

- 更新 **平均睡眠时间**  
- 睡眠越久 → **Bonus 越高** → 动态优先级越高  
- 保证等 I/O 的进程唤醒后 **优先被调度**

---

### 三、`try_to_wake_up()` — 唤醒

将睡眠/停止的进程：

1. 状态 → **`TASK_RUNNING`**  
2. 插入目标 CPU 的 **`runqueue`**

→ 等待队列：[Ch 3](../chapter-03-processes/notes/section-4-组织与查找.md)

---

### 四、`schedule()` — 调度核心

| 触发方式 | 场景 |
|----------|------|
| **直接调用** | 进程等资源 **主动阻塞** |
| **延迟调用** | 检测到 **`TIF_NEED_RESCHED`**（如 tick 返回、syscall 返回） |

**做什么：**

1. 在本地 `runqueue` 的 **活动数组** 中选 **最高优先级** 进程  
2. **`context_switch()`** — 切换地址空间 + 硬件上下文  

→ 切换细节：[Ch 3](../chapter-03-processes/notes/section-5-进程切换.md) · TLB：[Ch 2](../chapter-02-memory-addressing/notes/section-6-内存布局与TLB.md)

---

← [3. 数据结构](./section-3-调度器数据结构.md) · 下一节 [5. SMP 平衡](./section-5-SMP运行队列平衡.md)
