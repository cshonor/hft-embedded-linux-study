# 10.6 自定义 Panic Handler

> 🔴 精读

## 本节要点

### panic_notifier_list

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

### HFT 关联

HFT 系统可在 panic handler 中：发送告警通知运维、保存交易状态到持久存储、关闭网卡避免错误交易继续发送。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** panic handler 中有哪些限制？

> Panic 后系统状态不确定，handler 中不能：1) 分配内存（slab 可能已损坏）；2) 获取锁（可能死锁）；3) 依赖正常内核功能。只能做最简单的操作：写 I/O 端口、写 NVRAM、发送网络包（如果网络栈还可用）。建议 handler 极简，只做必要通知。


**Q:** 如何自定义 panic handler？有什么注意事项？

> 注册 panic_notifier_list 链表：`atomic_notifier_chain_register(&panic_notifier_list, &my_notifier)`。注意：(1) handler 在 panic 上下文运行，不能睡眠/分配内存；(2) 保持简短，不要做复杂操作；(3) 用于记录关键状态到 NVRAM 或发送告警。

</details>

## 交叉引用

- [05.6 ch10 panic](chapter-10-panic-lockup/notes/section-10-1.md)
