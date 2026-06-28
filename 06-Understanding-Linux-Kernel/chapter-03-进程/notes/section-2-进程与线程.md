## 2. 进程、轻量级进程与线程

---

### 一、进程是什么

**进程** = 「执行中程序的实例」— OS 调度和资源分配的基本单位。

---

### 二、Linux 的 LWP 模型

Linux **没有**像传统 Unix 那样严格区分「进程 vs 线程」，而是引入 **轻量级进程（LWP, Lightweight Process）**：

| 特点 | 说明 |
|------|------|
| 多线程应用 | 由**一组 LWP** 组成 |
| 共享资源 | 同一组内共享 **内存地址空间**、打开文件等 |
| 独立调度 | 每个 LWP 仍可被内核单独调度（modern 下即 `task_struct`） |

→ POSIX 线程（pthread）在 Linux 上底层就是 **clone + 共享 VM**。

---

### 三、线程组（Thread Group）与 PID

POSIX 要求：**同一多线程应用内所有线程共享同一个 PID**。

Linux 做法：

| 字段 | 含义 |
|------|------|
| **PID** | 每个 LWP 仍有唯一内核 PID |
| **`tgid`（Thread Group ID）** | 线程组 ID = **领头线程的 PID** |
| `getpid()` | 用户看到的其实是 **`tgid`**，不是单个 LWP 的 pid |

这样对外符合 POSIX，对内仍可独立调度每个线程。

---

### 四、和后续章的关系

| 主题 | 章节 |
|------|------|
| `task_struct` 细节 | [section-3](./section-3-进程描述符.md) |
| `clone()` 创建线程 | [section-6](./section-6-创建与销毁.md) |
| 线程调度 | [Ch 7 进程调度](../../chapter-07-进程调度.md) |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 进程描述符](./section-3-进程描述符.md)
