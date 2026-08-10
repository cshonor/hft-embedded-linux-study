# 3.1 printk 基础与日志级别

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

### printk 日志级别

| 级别 | 宏 | 数值 | 用途 |
|------|-----|------|------|
| KERN_EMERG | `pr_emerg()` | 0 | 系统不可用 |
| KERN_ALERT | `pr_alert()` | 1 | 必须立即处理 |
| KERN_CRIT | `pr_crit()` | 2 | 严重条件 |
| KERN_ERR | `pr_err()` | 3 | 错误条件 |
| KERN_WARNING | `pr_warn()` | 4 | 警告 |
| KERN_NOTICE | `pr_notice()` | 5 | 正常但重要 |
| KERN_INFO | `pr_info()` | 6 | 信息 |
| KERN_DEBUG | `pr_debug()` | 7 | 调试 |

### 控制台日志级别

```bash
# 查看/设置控制台日志级别
cat /proc/sys/kernel/printk
# 4 4 1 7
# ^ ^ ^ ^
# | | | └── default (默认日志级别)
# | | └──── minimum (最小日志级别)
| | └────── boot_default (启动默认)
# └──────── current (当前控制台级别)

# 只有级别 < current 的消息会打印到控制台
echo 8 > /proc/sys/kernel/printk  # 打印所有级别
echo 1 > /proc/sys/kernel/printk  # 只打印 EMERG/ALERT
```

### printk 的关键特性

```c
// 带级别前缀
printk(KERN_ERR "device %s failed: %d\n", dev->name, err);

// 推荐：使用 pr_xxx 简写
pr_err("device %s failed: %d\n", dev->name, err);
pr_warn("low memory: %lu KB free\n", free_kb);
pr_info("driver loaded successfully\n");

// 带 KERN_CONT 续行
printk(KERN_ERR "Error: ");
printk(KERN_CONT "code=%d\n", err);  // 续在同一行
```

### printk vs printf

| 特性 | printk | printf |
|------|--------|--------|
| 上下文 | 任何上下文（含中断） | 仅进程上下文 |
| 线程安全 | 是（锁保护环形缓冲区） | 是 |
| 中断安全 | 是 | N/A |
| 格式化 | 大部分相同 | 标准 C |
| %pK | 内核特有（需 CAP_SYSLOG） | 无 |
| 浮点 | ❌ 不支持 | ✅ |

## HFT 关联

HFT 内核模块应大量使用 `pr_debug()` 而非 `pr_err()`，通过 dynamic debug 在生产环境按需开关。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** printk 在中断上下文中安全吗？为什么？

> 安全。printk 使用自旋锁保护环形缓冲区，且 lockdep 标记为中断上下文安全的锁。但 printk 在中断上下文中应尽量简短（避免大量输出），因为长时间持有锁会影响其他 CPU 的 printk。

**Q2:** `pr_debug()` 和 `printk(KERN_DEBUG ...)` 完全等价吗？

> 不完全等价。`pr_debug()` 在未启用 `CONFIG_DYNAMIC_DEBUG` 时仅在 `DEBUG` 宏定义时编译生效。启用 `CONFIG_DYNAMIC_DEBUG` 后，`pr_debug()` 可通过 dyndbg 动态开关。`printk(KERN_DEBUG ...)` 始终编译生效，无法动态关闭。


**Q:** printk 的日志级别 KERN_DEBUG 和 KERN_INFO 在生产内核中有什么区别？

> KERN_DEBUG（7）默认不输出到控制台（console_loglevel 默认 < 7）。KERN_INFO（6）在大多数配置下输出。但两者都写入环形缓冲区（dmesg 可查），只是控制台输出行为不同。Dynamic Debug 可以运行时启用特定 KERN_DEBUG 消息。

**Q:** printk 在中断上下文中调用有什么风险？

> printk 获取 logbuf lock（spinlock），如果在中断上下文中调用且中断被持有 lock 的 CPU 触发，可能死锁。6.x 改为 lockless printk（printk-safe）缓解此问题。但在硬中断中仍应避免大量 printk，改用 printk_deferred() 或 trace_printk()。

</details>

## 交叉引用

- [05.6 ch03 dynamic debug](chapter-03-printk/notes/section-3-3.md)
