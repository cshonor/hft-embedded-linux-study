## ③ 链表结构 · 告别数组移位

Day 12 **数组 + `next` 全局最近超时** 仍可能在 **超时删除** 时：

```c
/* 朴素：最前定时器到期 */
for (i = 0; i < n-1; i++)
    timer[i] = timer[i+1];   /* 全体前移 — O(n) */
```

定时器 **500 个** × **100Hz** 检查 → **移位极贵**。

#### 链表版 `TIMER`

在 **`TIMER` 结构体加 `next` 指针** — 按 **超时先后顺序** 串成 **有序链表**：

| 操作 | 数组移位 | 链表 |
|------|----------|------|
| 超时弹出 | O(n) memmove | **`prev->next = cur->next`** — O(1) |
| 插入新定时器 | 可能 O(n) | **找位置改指针** — 无 bulk 移动 |

```
timerctl.head ──► T1 ──► T3 ──► T7 ──► …
                  next   next   next
```

**内存中的 TIMER 对象不搬家** — 只 **改链接** — ISR **时间骤降**。

→ [01-CSAPP Ch12 链表 vs 数组](../../../../01-CSAPP-3rd/chapter-12-concurrent-programming/) · Linux **`hrtimer` 红黑树/堆** 是更高级版

---
