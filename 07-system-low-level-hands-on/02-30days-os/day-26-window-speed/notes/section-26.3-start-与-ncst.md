## ③ start 与 ncst · 启动命令

#### `start`（仿 Windows）

**需求：** **保留当前 Console**，**另开环境** 跑程序。

```
start color2
    → 自动 new Console
    → 在新窗执行 color2.hrb
```

**一边调试/一边对比** — 多 **Shell 会话**。

#### `ncst`（no console start）

**纯 GUI app**（如 **color 测试窗**）— 背后 **黑 Console 碍眼**。

| 命令 | 行为 |
|------|------|
| **`ncst xxx`** | **分配 task、跑 xxx.hrb** · **不创建可见 Console** |

**配套修改：**

- API **无 Console 时不往虚空 putchar**
- app **结束 → 自动 task 终止** — 防 **孤儿任务**

**类似 Windows `start /B` 或 GUI-only 进程** — 桌面更干净。

→ [Day 20 cmd_app](../day-20-api/) · [Day 25 TASK.cons](../day-25-multi-console/)

---
