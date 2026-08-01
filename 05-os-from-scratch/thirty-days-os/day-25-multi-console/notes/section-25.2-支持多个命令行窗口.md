## ② 支持多个命令行窗口

**需求：** 两个 **不同色测试程序** **并排对比** → 需 **同时开多个 Console + 各跑一个 app**。

#### `cons[]` 数组

`bootpack.c` 中 **单 `cons` → `cons[N]`** — 多 **Console 任务/窗口**。

#### Bug：输出总进同一窗

**原因：** 全局 **唯一 cons 指针** — 各 app **不知道自己的 Console**。

#### 修复 · TASK 私有上下文

| 字段（示意） | 作用 |
|--------------|------|
| **`task->cons`** | 本任务绑定的 **控制台** |
| **`task->ds_base`** | 本 app **数据段基址**（衔接 Day 21 **0xfe8** 思路） |

**键入/输出/API 字符串** 均走 **当前 task 的 cons + ds** — **多 Console 真正隔离**。

→ [Day 17 console_task](../day-17-console/) · [Day 24 key_win](../day-24-window-ops/)

---
