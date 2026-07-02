## 3. 信号相关的数据结构

> POSIX 要求 **线程组共享** 信号处置 — 内核用多层结构追踪

---

### 一、`signal_struct` — 信号描述符

| 职责 | 内容 |
|------|------|
| **线程组级** | 整个线程组 **共享挂起信号** |
| **引用** | 同组线程共享同一 `signal_struct` |

→ 线程模型：[Ch 3 section-1](../../chapter-03-processes/notes/section-1-本章定位.md)

---

### 二、`sighand_struct` — 信号处理描述符

描述 **每种信号如何处理**：

| 动作 | 含义 |
|------|------|
| **`SIG_DFL`** | 默认动作 |
| **`SIG_IGN`** | 忽略 |
| **自定义 handler** | 用户态函数地址 |

线程组 **共享** `sighand_struct` — `sigaction()` 影响整组。

---

### 三、挂起信号队列

内核维护 **两个 pending 层次**：

| 队列 | 归属 |
|------|------|
| **共享挂起队列** | 线程组 — 在 `signal_struct` 中 |
| **私有挂起队列** | 单个 LWP — `task_struct.pending` |

生成信号时根据目标选择 **group** 或 **specific** 发送路径。

---

### 四、阻塞掩码 (Blocked Mask)

除 pending 外，每个线程还有 **阻塞信号集** — 被阻塞的信号 **可挂起但不传递**，直到解除阻塞。

→ 用户态 API：`sigprocmask()`（见 [section-6](./section-6-系统调用重启与相关syscall.md)）

---

← [2. 生命周期](./section-2-信号作用与生命周期.md) · 下一节 [4. 生成与传递](./section-4-生成与传递.md)
