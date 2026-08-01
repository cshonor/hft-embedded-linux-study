## ③ 字符输入 · 退格 · 每任务 FIFO

Day 13 **全局统一 FIFO** 收键鼠；Day 17 **输入投递** 要 **按任务分流**。

#### FIFO 绑在 `struct TASK`

```c
/* 示意 */
struct TASK {
    struct FIFO32 fifo;  /* 该任务私有输入队列 */
    …
};
```

**路径：**

```
键盘中断 → 全局/主路径收 scancode
    → keytable 转字符
    → Put 进 **key_to 指向任务的 fifo**
console_task / HariMain 从 **自己的 fifo Get** 处理
```

#### Console 能力

| 功能 | 实现要点 |
|------|----------|
| **收字存缓冲** | 从 **本任务 fifo** 读 |
| **显示** | 自绘字符（Day 5/14） |
| **Backspace** | 字符码 **8** — 删上一字符 |

**解耦：** 主程序 **只负责路由**；Console **只读自己的 fifo** — 像 **多终端各读各的 stdin**。

→ [Day 14 keytable](../day-14-keyboard/) · [Day 13 FIFO 编码段](../day-13-timer2/)

---
