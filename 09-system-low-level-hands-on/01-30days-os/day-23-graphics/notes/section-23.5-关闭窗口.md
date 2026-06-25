## ⑤ 关闭窗口 · 强制结束自动清理

#### 旧 bug

app **退出或死循环被强杀** → **窗口 sheet 残留** — 占 **内存 + 图层槽**。

#### EDX=14

**主动 `close_window(句柄)`** — app 正常路径释放。

#### SHEET 归属 TASK

**`struct SHEET` 记录所属 `TASK*`**：

| 场景 | OS 行为 |
|------|---------|
| app **RETF 正常退出** | 扫 **该 task 的 sheet** → 释放 |
| **Shift+F1 强杀**（Day 22） | 同样 **按 task 清窗** |

**生命周期：** **task 消亡 → 其 GUI 资源一并回收** — 像 **进程 exit 关 fd**。

→ [Day 22 Shift+F1](../day-22-c-apps/) · [Day 11 SHEET](../day-11-window/)

---
