## ④ 键盘输入 · walk.hrb（EDX=15）

| EDX | 功能 |
|-----|------|
| **15** | **读键** — 无键时可 **sleep** 省 CPU |

**`walk.hrb`：** 小键盘 **方向键** 移动窗口内 **`'*'`** — **图形 + 输入闭环**。

```
loop:
    k = api_getkey();   /* 阻塞/休眠 */
    更新 (x,y); api_point / api_refresh;
```

**里程碑：** **RPG / 小工具** 级交互 — 不再只是 **Console 打字**。

→ [Day 17 键路由](../day-17-console/) · [Day 16 sleep/wake](../day-16-multitask2/)

---
