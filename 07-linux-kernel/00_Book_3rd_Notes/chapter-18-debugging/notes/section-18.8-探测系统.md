## ⑦ 探测系统 · Poking and Probing

#### 用 UID 做条件开关

重写核心路径时：

```c
if (current_uid().val != 7777)
    old_fork_path();
else
    new_fork_path();   /* 仅测试用户走新代码 */
```

| 目的 | 新代码 bug **不拖垮全体用户** |

#### 限制打印频率

| 手段 | 说明 |
|------|------|
| **`printk_ratelimit()`** | 限制 **同一消息** 打印速率 |
| **发生次数限制** | 静态计数 — **仅前 N 次** `printk` |

| 问题 | 高频 ISR 里 `printk` → **控制台洪水** → **系统卡死** |

→ **Ch 7** ISR 要快 · **Ch 2** 不要用 `printf`

---
