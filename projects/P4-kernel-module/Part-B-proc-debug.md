# P4 Part B — /proc 统计 + kmalloc 追踪 + 内核调试

> 在 Part A 的字符设备上加 kmalloc 分配追踪和 /proc 统计，再故意写一个 bug 用 KASAN/Oops 定位。
> **做法：项目驱动，[`05.6`](../../05.6-kernel-debugging/) / [`06`](../../06-linux-mm/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [08.6 ch03 printk](../../05.6-kernel-debugging/chapter-03-printk/notes/) | printk 日志级别、rate limiting |
| [08.6 ch05 KASAN](../../05.6-kernel-debugging/chapter-05-memory-debug-1/notes/) | 内存越界/释放后使用检测 |
| [08.6 ch07 Oops](../../05.6-kernel-debugging/chapter-07-oops/notes/) | 内核崩溃日志怎么读 |
| [08.6 ch09 Ftrace](../../05.6-kernel-debugging/chapter-09-ftrace/) | 函数级追踪 |
| [09 Slab 分配器](../../06-linux-mm/chapter-08-slab-allocator/) | kmalloc 底层 = slab/slub |

---

## Phase 1：kmalloc 分配追踪（1 小时）

### 做什么

在字符设备里用 kmalloc 分配多个缓冲区，记录每次分配的大小/调用点/时间戳。

### 代码骨架

```c
// 在 Part A 的 chardev.c 基础上扩展
#include <linux/ktime.h>

#define MAX_ALLOCS 256

struct alloc_record {
    size_t size;
    void *addr;
    ktime_t timestamp;
    int line;  // 调用点
};

static struct alloc_record alloc_log[MAX_ALLOCS];
static atomic_t alloc_count = ATOMIC_INIT(0);

// 封装 kmalloc：记录每次分配
static void *tracked_kmalloc(size_t size, int line) {
    void *p = kmalloc(size, GFP_KERNEL);
    if (!p) return NULL;

    int idx = atomic_inc_return(&alloc_count) - 1;
    if (idx < MAX_ALLOCS) {
        alloc_log[idx].size = size;
        alloc_log[idx].addr = p;
        alloc_log[idx].timestamp = ktime_get();
        alloc_log[idx].line = line;
    }
    pr_info("kmalloc: size=%zu addr=%px line=%d\n", size, p, line);
    return p;
}

// 用宏自动传行号
#define tracked_alloc(sz) tracked_kmalloc(sz, __LINE__)

// ioctl 接口：清零统计
#define IOCTL_CLEAR_STATS _IO('M', 1)
#define IOCTL_SET_BUFSIZE _IOW('M', 2, int)

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case IOCTL_CLEAR_STATS:
        atomic_set(&alloc_count, 0);
        memset(alloc_log, 0, sizeof(alloc_log));
        break;
    case IOCTL_SET_BUFSIZE: {
        int new_size;
        if (copy_from_user(&new_size, (void __user *)arg, sizeof(int)))
            return -EFAULT;
        // 重新分配
        kfree(kernel_buffer);
        kernel_buffer = tracked_alloc(new_size);
        if (!kernel_buffer) return -ENOMEM;
        break;
    }
    default:
        return -ENOTTY;
    }
    return 0;
}
```

### 分步实现

1. **定义 `alloc_record` 数组**：记录 size/addr/timestamp/line
2. **封装 `tracked_kmalloc`**：`#define tracked_alloc(sz)` 用 `__LINE__` 自动传行号
3. **ioctl 接口**：`IOCTL_CLEAR_STATS` 清零统计，`IOCTL_SET_BUFSIZE` 动态改缓冲区大小
4. **在 fops 里加 `.unlocked_ioctl = my_ioctl`**

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| `__LINE__` 在宏里展开不对 | 所有记录都是同一行 | 用 `#define` 不用 inline 函数传 `__LINE__` |
| ioctl 命令编码冲突 | 行为诡异 | 用 `_IO`/`_IOW`/`_IOR` 宏定义命令号 |
| atomic_t 用 ++ | 编译错误 | 内核用 `atomic_inc_return()` |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| kmalloc/slab 原理 | [09 ch08 Slab](../../06-linux-mm/chapter-08-slab-allocator/) |
| GFP 标志 | [09 ch06 物理页分配](../../06-linux-mm/chapter-06-physical-page-allocation/) |

---

## Phase 2：/proc 统计接口（1 小时）

### 做什么

通过 `/proc/mydev_stats` 暴露：分配次数、总字节、峰值、读写计数。

### 代码骨架

```c
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static atomic_t read_count = ATOMIC_INIT(0);
static atomic_t write_count = ATOMIC_INIT(0);
static size_t peak_bytes = 0;
static DEFINE_SPINLOCK(stats_lock);

// seq_file 接口：逐行输出统计
static int stats_show(struct seq_file *m, void *v) {
    int count = atomic_read(&alloc_count);
    size_t total = 0;
    for (int i = 0; i < count && i < MAX_ALLOCS; i++)
        total += alloc_log[i].size;

    seq_printf(m, "alloc_count:  %d\n", count);
    seq_printf(m, "total_bytes:  %zu\n", total);
    seq_printf(m, "peak_bytes:   %zu\n", peak_bytes);
    seq_printf(m, "read_count:   %d\n", atomic_read(&read_count));
    seq_printf(m, "write_count:  %d\n", atomic_read(&write_count));
    seq_printf(m, "\nRecent allocations:\n");
    for (int i = 0; i < count && i < MAX_ALLOCS && i < 10; i++) {
        seq_printf(m, "  [%d] size=%zu addr=%px line=%d\n",
                   i, alloc_log[i].size, alloc_log[i].addr,
                   alloc_log[i].line);
    }
    return 0;
}

static int stats_open(struct inode *inode, struct file *file) {
    return single_open(file, stats_show, NULL);
}

static const struct proc_ops stats_fops = {
    .proc_open    = stats_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// 在 init 里创建 /proc 条目
proc_create("mydev_stats", 0444, NULL, &stats_fops);
// 在 exit 里删除
remove_proc_entry("mydev_stats", NULL);
```

### 分步实现

1. **在 read/write 回调里加计数**：`atomic_inc(&read_count)` / `atomic_inc(&write_count)`
2. **用 `seq_file` 接口**：比直接 `read` 实现更安全（不用管 offset/buffer 大小）
3. **`proc_create`** 创建 `/proc/mydev_stats`，权限 `0444`（只读）
4. **测试**：`cat /proc/mydev_stats`

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 用 `proc_ops` 还是 `file_operations` | 编译错误 | 5.6+ 用 `proc_ops`，旧内核用 `file_operations` |
| 忘了 `remove_proc_entry` | 卸载后 /proc 残留 | exit 必须删 |
| `seq_printf` 格式符 | 编译警告 | 内核用 `%zu` 不一定支持，用 `%lu` + cast |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| /proc / seq_file | [LKD 17.3 sysfs](../../05-linux-kernel/00_Book_3rd_Notes/chapter-17-devices-modules/notes/section-17.3-sysfs-虚拟文件系统.md) |
| 现代 proc API | [05.5 设备驱动](../../05.5-modern-kernel/chapter-08-device-driver-dt/) |

---

## Phase 3：故意写 bug，用 KASAN/Oops 定位（1-2 小时）

### 做什么

在模块里故意写 3 个经典 bug，用内核调试工具定位修复。

### Bug 1：越界写（KASAN 检测）

```c
// 故意越界：分配 100 字节，写 120
static void bug_oor_write(void) {
    char *p = kmalloc(100, GFP_KERNEL);
    memset(p, 0, 120);  // 越界 20 字节
    kfree(p);
}
```

**启用 KASAN**：内核配置 `CONFIG_KASAN=y`，重新编译内核（或用支持 KASAN 的发行版内核）。加载模块后调用 `bug_oor_write()`，`dmesg` 会输出 KASAN 报告，精确到行号。

### Bug 2：释放后使用（UAF）

```c
// 故意 UAF：free 后还读
static void bug_uaf(void) {
    char *p = kmalloc(64, GFP_KERNEL);
    kfree(p);
    pr_info("UAF: reading freed memory: %c\n", p[0]);  // 危险！
}
```

### Bug 3：空指针解引用（Oops）

```c
// 故意空指针：触发 Oops
static void bug_null_ptr(void) {
    int *p = NULL;
    *p = 42;  // 内核立即 Oops
}
```

### 调试流程

1. **Oops 日志**：`dmesg` 看到 `Unable to handle kernel NULL pointer dereference`
2. **解码地址**：`addr2line -e mymod.ko <address>` 或 `objdump -d mymod.ko`
3. **Ftrace 追踪**：
   ```bash
   echo function > /sys/kernel/debug/tracing/current_tracer
   echo my_open > /sys/kernel/debug/tracing/set_ftrace_filter
   cat /sys/kernel/debug/tracing/trace
   ```
4. **KASAN 报告**：看 `BUG: KASAN: slab-out-of-bounds in bug_oor_write+0x2c/0x40`，直接定位到行号

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| KASAN 没启用 | 越界写没报 | 需要内核编译时开 `CONFIG_KASAN` |
| Oops 后系统卡死 | 只能硬重启 | 空指针 = 立即 panic（除非配了 panic_on_oops=0）|
| 看不懂 Oops 地址 | 不知道哪行 | 用 `addr2line` 或 `gdb vmlinux` 解析符号 |
| Ftrace 权限 | Permission denied | 需要 `sudo` 或 root |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| printk 调试 | [08.6 ch03 printk](../../05.6-kernel-debugging/chapter-03-printk/notes/) |
| KASAN 内存检测 | [08.6 ch05](../../05.6-kernel-debugging/chapter-05-memory-debug-1/notes/) |
| Oops 日志分析 | [08.6 ch07 Oops](../../05.6-kernel-debugging/chapter-07-oops/notes/) |
| Ftrace 函数追踪 | [08.6 ch09 Ftrace](../../05.6-kernel-debugging/chapter-09-ftrace/) |
| Kprobes 动态追踪 | [08.6 ch04 Kprobes](../../05.6-kernel-debugging/chapter-04-kprobes/notes/) |

---

## 完整测试流程

```bash
# 编译加载
make && sudo insmod chardev.ko

# 字符设备
sudo mknod /dev/mydev c $(awk '/mydev/{print $1}' /proc/devices) 0
sudo chmod 666 /dev/mydev

# 用户态测试
echo "test data" > /dev/mydev
cat /dev/mydev

# 统计
cat /proc/mydev_stats

# ioctl 测试（需要用户态程序调用 ioctl）
./user_test --clear-stats
./user_test --set-bufsize 8192

# 调试
dmesg | tail -20
sudo cat /sys/kernel/debug/tracing/trace

# 清理
sudo rmmod chardev
sudo rm /dev/mydev
```

← [P4 索引](./README.md) · [08.6 模块](../../05.6-kernel-debugging/) · [09 模块](../../06-linux-mm/)
