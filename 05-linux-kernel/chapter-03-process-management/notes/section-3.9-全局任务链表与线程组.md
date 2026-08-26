# 3.9 全局任务链表与线程组链表

> `task_struct.tasks` 把所有进程组长串成全局双向循环链表；`thread_group` 串同进程内全部线程。两条链配合才能枚举系统所有线程。
> 数据结构层面：`list_head` 双向循环 + RCU 保护；与 CFS 运行队列（红黑树）完全是两套东西。

---

## 一、字段与表头

```c
struct task_struct {
    struct list_head tasks;        // 挂入全局任务链表的节点
    struct list_head thread_group; // 挂入线程组链表的节点
    // ...
};
```

| 要点 | 说明 |
|------|------|
| 链表头 | `init_task`（pid=0，0 号 CPU 的 idle/swapper），**编译期静态分配**于 `init/init_task.c`，不是 malloc |
| 表头变量 | `&init_task.tasks`——全局任务链的 head |
| 节点含义 | 链上每个 `tasks` 成员对应一个 task_struct；**只有不带 `CLONE_THREAD` 的进程（线程组长）挂这条链** |
| 串联接口 | `list_add_tail_rcu(&p->tasks, &init_task.tasks)`（`copy_process()` 中） |
| 删除接口 | `list_del_rcu(&p->tasks)` + RCU 延迟 `put_task_struct_rcu_user()` |

> 区分点不是"组长"概念，而是 **`CLONE_THREAD` flag**：fork() 无 CLONE_THREAD → 挂 `init_task.tasks`；pthread_create() 有 CLONE_THREAD → 挂 leader 的 `thread_group`。

---

## 二、两条链表对照

| 链表 | 字段 | 表头 | 存什么 |
|------|------|------|--------|
| 全局任务链 | `task_struct.tasks` | `&init_task.tasks` | 各进程组长（每进程 1 个） |
| 线程组链 | `task_struct.thread_group` | leader 的 `thread_group`（自环） | 同进程内全部线程（含组长） |

举例：pthread 创建 4 个线程（1 主 + 3 子）
- `init_task.tasks` 上只多 **1 条**：主线程（组长）
- 组长的 `thread_group` 链串联全部 **4 条** task_struct

### pid vs tgid

| 字段 | 含义 |
|------|------|
| `pid` | 每个 task_struct（每个线程）的唯一 ID |
| `tgid` | 线程组 ID，等于组长的 `pid` |

- 组长：`pid == tgid`
- 子线程：`pid != tgid`，`tgid` 等于组长 `pid`

> 判断一个 task 是否线程组组长：`(p->pid == p->tgid)`。`fork()` 出来的进程 `tgid = pid`（自己是组长）；`clone(CLONE_THREAD)` 出来的子线程 `tgid` 继承组长 `pid`。

> 用户态看到的"PID"实际是内核的 `tgid`（`getpid()` 返回 tgid），`gettid()` 才返回真正的 pid。

---

## 三、遍历：三个现成宏

内核不裸写 `list_for_each_entry`，`include/linux/sched.h` 提供三个宏：

```c
for_each_process(p)           // 遍历所有进程组长（不含子线程）
for_each_thread(p, t)         // 遍历 p 所在线程组的全部线程
for_each_process_thread(p, t) // 双层嵌套，遍历系统全部线程
```

`for_each_process` 的定义——注意 `init_task` 是**哨兵头**，遍历从它出发、遇它即止，**`init_task` 自身不被输出**：

```c
#define for_each_process(p) \
    for (p = &init_task ; (p = next_task(p)) != &init_task ; )
```

要遍历**每个线程**（含子线程），正确写法是 `for_each_process_thread`，不是裸 `list_for_each_entry(..., &init_task.tasks, tasks)`。

---

## 四、idle task 是 per-cpu 的

`init_task` 只是 **0 号 CPU 的 idle（swapper）**。SMP 上每 CPU 都有独立 idle task，由 `fork_idle(cpu)` 在 `kernel/smpboot.c` 的 `idle_threads_init()` 里从 `init_task` 复制创建——**这些 per-cpu idle 也挂在 `init_task.tasks` 全局链上**。

推论：链表上其实有 N 个 pid=0 的 idle task（N=CPU 数），`for_each_process` 会遍历到非 0 号 CPU 的 idle；但 `ps` 不显示 pid=0。

---

## 五、与 procfs 的关系（"ps 读这条链"的真相）

ps/top 读 `/proc`，procfs 是中间层，全局链是**最终后端**：

```
ps/top  ──读→  /proc  readdir
                 │
                 ▼  procfs 后端
        proc_pid_readdir()  ──→  find_ge_pid() ──→ for_each_process  (tasks 链)
        /proc/[pid]/task/   ──→  next_tid()     ──→ thread_group 链
```

- 列进程：`/proc` readdir → `for_each_process` 拿 tgid
- 列某进程线程：`/proc/[pid]/task/` readdir → `thread_group` 拿 tid

---

## 六、RCU：读多写少

全局链被读频繁（ps、top、procfs、内核自身 `for_each_process`），修改少（fork/exit）：

- **读端**：RCU 读临界区内无锁遍历，靠 RCU 保证节点不中途释放
- **写端**：增删用 `_rcu` 版本
  ```c
  list_add_tail_rcu(&p->tasks, &init_task.tasks);   // fork
  list_del_rcu(&p->tasks);                           // exit
  put_task_struct_rcu_user(p);                       // RCU 宽限期后才 __put_task_struct
  ```
- **约束**：读端不能把 task_struct 引用持有跨出 RCU 读临界区，否则节点可能已被 free

---

## 七、和 CFS 运行队列的关系（别混淆）

| 数据结构 | 用途 |
|----------|------|
| `init_task.tasks` 全局链 | **枚举/遍历**系统全部进程，和调度无关 |
| CFS 运行队列（红黑树 `cfs_rq.tasks_timeline`） | 调度器**选下一个进程运行**用 |

两者完全不是一套数据结构。进程既在全局 tasks 链上（供枚举），又在某个运行队列里（供调度）；两套独立链表节点。

---

## 八、mermaid 示意

```mermaid
graph LR
    HEAD["init_task.tasks (head)"] -.哨兵.-> A

    subgraph 全局任务链 init_task.tasks
      A["进程1 组长<br/>tasks"] === A1
      A1 --> A2["进程2 组长<br/>tasks"]
      A2 --> IDLE["CPU1 idle<br/>pid=0<br/>tasks"]
      IDLE -.循环.-> HEAD
    end

    subgraph 进程1的线程组链 thread_group
      A -.leader自环.-> A
      A --> T1["线程1<br/>thread_group"]
      T1 --> T2["线程2<br/>thread_group"]
      T2 -.循环.-> A
    end

    A -.可调度.> RB["CFS rq 红黑树<br/>vruntime 排序"]
    T1 -.可调度.> RB
```

---

## 九、面试坑点

1. ❌误区："系统所有线程全部挂在 `init_task.tasks` 全局链"
   ✅事实：只有线程组长；子线程不在 tasks 链。遍历全部线程用 `for_each_process_thread` 或组长 + `for_each_thread`。

2. ❌误区：`init_task` 是内核启动后动态分配
   ✅事实：静态编译生成，pid=0 idle；且只是 0 号 CPU 的 idle，其他 CPU 的 idle 由 `fork_idle` 动态创建并挂同一链。

3. ❌误区：`for_each_process` 会遍历到 `init_task` 自己
   ✅事实：`init_task` 是哨兵头，遍历起点兼终点，自身不被输出。

4. ❌误区：`tasks` 链和 CFS 运行队列是一回事
   ✅事实：tasks 是枚举用的双向链表；CFS rq 是调度用的红黑树。两套独立。

---

### Quiz

> Q：在内核里想遍历机器上**每一个线程（含子线程）**，只用 `list_for_each_entry(..., &init_task.tasks, tasks)` 行不行？
> A：不行。这个循环只拿到各进程组长，拿不到同进程创建的轻量级线程。正确做法：用 `for_each_process_thread(p, t)`，或对每个组长再 `for_each_thread(leader, t)` 遍历它的 `thread_group` 链。

> Q：`init_task.tasks` 链表上有几个 pid=0 的 task？
> A：CPU 数个。`init_task` 是 0 号 CPU 的 idle（静态）；其余 CPU 的 idle 由 `fork_idle(cpu)` 动态创建并挂同一链，`for_each_process` 会遍历到它们。

> Q：调用 `clone(CLONE_THREAD)` 创建子线程，子线程的 `tasks` 链表节点处于什么状态？
> A：该 `list_head` 变量存在于 task_struct 内存里，**但不插入任何链表，处于游离未挂载状态**，不参与全局 tasks 遍历。只有 `thread_group` 节点被挂到组长的链上。

> Q：用户态 `getpid()` 和 `gettid()` 返回的分别是什么？
> A：`getpid()` 返回 `tgid`（线程组 ID，即组长 pid）；`gettid()` 返回真正的 `pid`（每个线程唯一）。所以主线程里 `getpid() == gettid()`，子线程里 `getpid() != gettid()`。
