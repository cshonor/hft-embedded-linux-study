## ⑥ 动态定时器 · Dynamic Timers

**推迟执行** 一段 **jiffies** 后再跑回调 — **一次性**，到期 **自动销毁**。

| 结构 | `struct timer_list` |
|------|---------------------|
| 初始化 | **`init_timer()`**（书中 API） |
| 过期时刻 | **`expires`**（jiffies） |
| 回调 | **`function`** |
| 激活 | **`add_timer()`** |
| 改期 | **`mod_timer()`** |
| 同步删除 | **`del_timer_sync()`** — 等回调跑完 |

#### 执行上下文

| 事实 | 说明 |
|------|------|
| 下半部 | 作为 **`TIMER_SOFTIRQ`** **异步** 执行 |
| 约束 | **不可睡眠**（同 softirq）— 回调须短 |

```c
void my_timer_fn(unsigned long data) { /* 快速完成 */ }

setup: expires = jiffies + HZ;  /* 约 1 秒后 */
       function = my_timer_fn;
add_timer(&timer);
```

→ **Ch 8** softirq

---
