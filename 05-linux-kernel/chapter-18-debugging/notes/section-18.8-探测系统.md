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



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 如何用条件 UID 在生产环境安全调试？

<details><summary>答案</summary>

技巧：代码中加 `if (current->uid == DEBUG_UID) printk(...)`。生产环境正常运行不打印，需要调试时用 `setuid DEBUG_UID` 运行测试程序 → 触发调试输出。这样不影响生产流量，且不需要重新编译内核。HFT 可用类似方法在特定测试账户的交易路径上启用 trace。

</details>

</details>
---
