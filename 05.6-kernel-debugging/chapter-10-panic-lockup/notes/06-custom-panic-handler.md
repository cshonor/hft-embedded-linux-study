# 自定义 Panic Handler

> 🔴 精读

## 概念详解

### panic_notifier_list

内核 panic 时会依次调用 `panic_notifier_list` 中注册的回调函数，允许自定义 panic 处理逻辑。

```c
#include <linux/panic_notifier.h>

static int my_panic_handler(struct notifier_block *nb,
                            unsigned long action, void *data) {
    // data 是 panic 的格式字符串
    pr_emerg("My panic handler: system going down\n");
    
    // 可以在此:
    // 1. 发送告警通知
    // 2. 保存崩溃信息到 NVRAM
    // 3. 关闭硬件设备
    
    return NOTIFY_DONE;
}

static struct notifier_block my_panic_nb = {
    .notifier_call = my_panic_handler,
    .priority = 1,  // 优先级 (越高越先调用)
};

static int __init my_init(void) {
    atomic_notifier_chain_register(&panic_notifier_list, &my_panic_nb);
    return 0;
}
static void __exit my_exit(void) {
    atomic_notifier_chain_unregister(&panic_notifier_list, &my_panic_nb);
}
```

### Panic Handler 中的限制

| 操作 | 能否执行 | 原因 |
|------|---------|------|
| 写 I/O 端口 | ✅ | 直接硬件操作 |
| 写 NVRAM | ✅ | 直接硬件操作 |
| 发送网络包 | ⚠️ | 网络栈可能已损坏 |
| 分配内存 | ❌ | slab 可能已损坏 |
| 获取锁 | ❌ | 可能死锁 |
| 睡眠/调度 | ❌ | 系统已停止 |
| printk | ✅ | 可用但可能丢失 |

### Priority 顺序

```c
// priority 越高越先调用
// 内核定义的优先级常量:
#define PANIC_PRIORITY_LOW      0
#define PANIC_PRIORITY_NORMAL   100
#define PANIC_PRIORITY_HIGH     200
#define PANIC_PRIORITY_HIGHEST  300

// HFT 模块建议用高优先级
// (在系统完全停止前执行)
static struct notifier_block my_panic_nb = {
    .notifier_call = my_panic_handler,
    .priority = PANIC_PRIORITY_HIGH,
};
```

### HFT 关联应用

HFT 系统可在 panic handler 中：

1. **发送告警通知运维**：通过简单网络包或 GPIO 信号
2. **保存交易状态**：写入 NVRAM 或持久存储
3. **关闭网卡**：避免错误交易继续发送
4. **记录时间戳**：帮助分析崩溃时间

```c
// HFT panic handler 示例
static int hft_panic_handler(struct notifier_block *nb,
                             unsigned long action, void *data) {
    // 1. 关闭交易网卡 (直接 I/O 端口操作)
    writel(0, net_base + NET_CTRL_REG);
    
    // 2. 保存最后交易状态到 NVRAM
    void __iomem *nvram = ioremap(NVRAM_BASE, 256);
    if (nvram) {
        writel(0xDEAD, nvram + 0);      // panic 标记
        writel(jiffies, nvram + 4);     // 时间戳
        memcpy_toio(nvram + 8, &last_order, sizeof(last_order));
        iounmap(nvram);
    }
    
    // 3. 点亮错误 LED
    gpio_set_value(ERROR_LED_GPIO, 1);
    
    // 注意: 不能分配内存、获取锁、睡眠
    return NOTIFY_DONE;
}
```

### Panic Handler 最佳实践

```c
// 1. 保持简短 — handler 执行时间影响重启延迟
// 2. 不依赖内核子系统 — 它们可能已损坏
// 3. 只做必要的紧急操作
// 4. 使用直接 I/O 操作而非内核 API
// 5. 测试 handler 在各种 panic 场景下的行为

// 测试 panic handler
// echo c > /proc/sysrq-trigger  — 触发 panic
```

### panic_notifier vs die_notifier

| 特性 | panic_notifier | die_notifier |
|------|---------------|--------------|
| 触发时机 | panic() 时 | Oops 时 |
| 系统状态 | 即将停止 | 可能继续运行 |
| 可用操作 | 极有限 | 较多 |
| 注册函数 | atomic_notifier_chain_register(&panic_notifier_list) | register_die_notifier() |
| HFT 用途 | 关闭设备、保存状态 | 记录错误信息 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** panic handler 中有哪些限制？

> Panic 后系统状态不确定，handler 中不能：分配内存（slab 可能已损坏）、获取锁（可能死锁）、依赖正常内核功能。只能做最简单的操作：写 I/O 端口、写 NVRAM、发送网络包（如果网络栈还可用）。建议 handler 极简。

**Q2:** 如何自定义 panic handler？

> 注册 panic_notifier_list 链表：`atomic_notifier_chain_register(&panic_notifier_list, &my_notifier)`。注意：(1) handler 在 panic 上下文运行，不能睡眠/分配内存；(2) 保持简短；(3) 用于记录关键状态到 NVRAM 或发送告警。

**Q3:** panic handler 的 priority 有什么作用？

> priority 决定 handler 的调用顺序——越高越先调用。在系统完全停止前，高优先级的 handler 先执行。HFT 模块建议用高优先级，确保在系统完全不可用前完成紧急操作。

**Q4:** HFT panic handler 为什么要关闭网卡？

> Panic 后系统状态不确定，如果网卡继续发送可能产生错误交易（如重复下单、错误价格）。在 panic handler 中通过直接 I/O 端口操作关闭网卡，确保不会继续发送交易。

**Q5:** `die_notifier` 和 `panic_notifier` 的区别？

> `die_notifier` 在 Oops 时触发（系统可能继续运行），可以记录错误信息。`panic_notifier` 在 panic 时触发（系统即将停止），只能做紧急操作。HFT 模块可以同时注册两者：Oops 时记录详细信息，panic 时关闭设备。

</details>

## 交叉引用

- [05.6 ch10 Panic 触发与处理](../../chapter-10-panic-lockup/notes/01-panic-causes.md)
- [05.6 ch07 Oops vs Panic](../../chapter-07-oops/notes/01-oops-vs-panic.md)
- [05.6 ch10 Kdump/Kexec](../../chapter-10-panic-lockup/notes/07-kdump-kexec.md)
