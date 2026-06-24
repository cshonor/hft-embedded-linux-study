## ④ GUI 编程 · 窗口与绘图 API

底层 **保护 + 装载** 就绪 → **扩展 INT 0x40 / EDX 功能号**（Day 20 路由）：

| API 能力（示意） | 说明 |
|------------------|------|
| **开窗口** | 返回 **窗口句柄**（id / sheet 绑定） |
| **窗口内字符** | 在 **指定句柄** 上 putchar |
| **画方块** | 矩形填充 — GUI 原语 |

**app 侧（C）：**

```c
win = api_open_window(...);
api_putchar(win, x, y, 'A', color);
api_box(win, x0, y0, x1, y1, color);
```

**OS 侧：** 复用 Day 10–11 **SHEET/窗口** — API **只是跨 Ring 的封装**。

→ [Day 11 make_window8](../day-11-window/) · [Day 20 EDX 路由](../day-20-api/)

---
