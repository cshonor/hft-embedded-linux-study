# Linux"一切皆文件"，调度类算不算文件？ — 修正版

> 结论先行：**调度类本身不是文件，是内核内存里的纯虚函数表；但调度器的可调参数（全局变量）通过 sysfs/kernfs 伪文件暴露成文件接口。**
> 注意一个关键区分：`sched_class` 定义**行为**（怎么调度），全局变量定义**参数**（调度多重）。

---

### 一、先分清两个概念

**1. 调度类（sched_class）**

内核 C 代码里的结构体对象，驻留内核虚拟内存。看它的定义（`kernel/sched/sched.h`）：

```c
struct sched_class {
    void (*enqueue_task)(...);
    void (*dequeue_task)(...);
    struct task_struct *(*pick_next_task)(...);
    void (*task_tick)(...);
    void (*switched_to)(...);
    ...
};
```

它是个**纯虚函数表（vtable）**——字段几乎全是函数指针，**不存放任何可调参数**。四个实例：

- `stop_sched_class`（stop 任务，最高优先级，迁移用）
- `dl_sched_class`（Deadline，EDF）
- `rt_sched_class`（实时，FIFO/RR）
- `fair_sched_class`（CFS/EEVDF）

> 内核运行时数据结构，不是磁盘文件，不是 inode，不能直接 open/read/write。
> → **它本身不算文件。**

**2. 可调参数是全局变量，不在 sched_class 里**

CFS 的参数是 `kernel/sched/fair.c` 里的**静态全局变量**：

```c
unsigned int sysctl_sched_latency = 6000000;          /* 6ms */
unsigned int sysctl_sched_min_granularity;
static unsigned int sysctl_sched_cfs_bandwidth_slice = 5000;  /* 5ms */
```

写 `/sys/kernel/sched/latency_ns` 改的就是这些全局变量——跟 `sched_class` 结构体没有关系。

**3. sysfs（/sys）伪文件**

"一切皆文件" ≠ 所有内核内部数据结构本身就是磁盘文件。真正含义：**很多内核对象对外包装成文件接口，用 read/write 系统调用交互**。

调度相关接口：

- `/sys/kernel/sched/latency_ns`、`cfs_bandwidth_slice_ns` 等 —— CFS 参数（**6.6+ 内核才有**；老内核在 `/proc/sys/kernel/sched_*`，sysctl 接口）
- `/sys/kernel/sched/features` —— 运行时开关 CFS 特性
- `/sys/devices/system/cpu/` —— CPU 拓扑/热迁移
- cgroup v2 的 `cpu.max` / `cpu.weight` —— cgroupfs 伪文件

这些是伪文件：open 时内核通过 show 回调从内存现场取值；write 走 store 回调直接改内核内存里的运行参数。

---

### 二、VFS 是怎么把 sysfs 伪文件和调度参数挂钩的

以 write `/sys/kernel/sched/latency_ns` 为例，完整调用链：

```
write(2)
 → ksys_write → vfs_write → call_write_iter
   → kernfs_fop_write_iter          /* sysfs 的 file_operations */
     → sysfs_kf_write
       → attr->store()              /* 注册时给的回调 */
         → 直接给 sysctl_sched_latency 赋值，完事
```

注册侧，内核用 `struct kobj_attribute` 把变量和 show/store 回调绑在一起：

```c
static ssize_t latency_ns_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned long val;
    /* sscanf → 写 sysctl_sched_latency */
}

static struct kobj_attribute latency_ns_attr =
    __ATTR(latency_ns, 0644, latency_ns_show, latency_ns_store);
```

底层是 **kernfs**（3.14 从 sysfs 拆出来的通用层）：`/sys/kernel/sched/` 下每个文件对应一个 `kernfs_node`；open 时才按需分配内存中的 inode/dentry，`stat` 看到的 inode 号是临时的。read 每次 show 现场从全局变量取值，write 走 store 直接改内存——**断电即失，重启恢复默认**。

---

### 三、HFT/低延迟真正相关的"调度类文件接口"

| 接口 | 作用 | HFT 用途 |
|------|------|----------|
| `/sys/kernel/sched/features` | 运行时开关 CFS 特性（NOHZ_IDLE、TTWU_QUEUE 等） | 排查调度抖动：`grep -v NOHZ /sys/kernel/sched/features` |
| cgroup v2 `cpu.max` | 写入直接改 `cfs_bandwidth` 的 quota/period | 交易进程资源隔离 |
| cgroup v2 `cpu.weight` | 改 `task_group` 权重 | 关键进程抢占保障 |
| `/proc/sys/kernel/sched_rt_runtime_us` | RT 带宽，默认 950000/1000000 | **SCHED_FIFO 也会被节流 5%**，低延迟场景常写 -1 关掉 |

注意：**给任务设置调度类本身不是文件接口**，走 `sched_setscheduler(2)` 系统调用（`chrt` 命令的底层）。"文件化"只覆盖参数调优，不覆盖策略指定。

---

### 四、对比表

| 对象 | 本身是不是文件 | 是否提供文件接口 |
|------|----------------|------------------|
| `struct sched_class` | ❌ 纯 vtable，内核内存 | ⚠️ 类本身没有；**参数**（全局变量）经 sysfs 可改 |
| 调度参数全局变量 | ❌ 静态全局变量 | ✅ sysfs/sysctl 伪文件 |
| 进程 task_struct | ❌ 内存结构体 | ✅ `/proc/[pid]/*` |
| 磁盘普通文件 | ✅ 真正磁盘文件 | ✅ |
| socket | ❌ socket 内核对象 | ✅ fd 指向 `struct file`，`f_op` 是 `socket_file_ops`。**无路径无磁盘 inode**（AF_UNIX bind 到路径才有）；连"伪文件"都算不上，只借了 fd 这层接口 |

---

### 五、一句话理解"一切皆文件"的真正边界

> 凡能挂上 `struct file` + `file_operations`、拿到 fd 的东西，对外就表现成文件。
> 内核内部的结构体、算法、调度类、进程、socket、信号量：本身不是文件。
> proc、sysfs、cgroupfs 全是内存文件系统，断电消失，只是把内核内存的数据伪装成文件给用户态。

**常见误区：**
- ❌ 内核里每样东西，硬盘上都有对应文件存着。
- ✅ 很多内核资源**抽象成文件系统接口**统一访问，不一定落磁盘。

---

### 小 quiz

**问：修改 `/sys/kernel/sched/*`，写进去数值，是写到硬盘吗？**

答：不是。write(2) 经 VFS → kernfs → `attr->store()` 回调，直接修改 `kernel/sched/fair.c` 里的全局变量（如 `sysctl_sched_latency`）；重启全部恢复默认，不会持久化。

**问：`/sys/kernel/sched/latency_ns` 写的值最终落在 `sched_class` 结构体的哪个成员里？**

答：哪个都不落。`sched_class` 是纯函数表，没有参数成员；写的是独立的静态全局变量 `sysctl_sched_latency`。
