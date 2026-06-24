## ③ 真正实现键盘输入 · keytable

Day 6–7 屏幕显示 **原始扫描码**（如 **A = 0x1E**）— 用户要 **字母**。

#### 查表法

定义 **`keytable[]`**（及 **shift 表** 等，以原书为准）：

```c
/* 示意：scancode → ASCII */
char c = keytable[scancode];
putchar(c, ...);
```

| 对比 | 数十个 `if (code==0x1E)` | **`keytable[code]`** |
|------|--------------------------|----------------------|
| 维护 | 难扩展 | **改表即可** |
| 速度 | 分支链 | **O(1) 数组读** |

FIFO 统一路径（Day 13）：键盘中断 → **Put(256+scancode)** → 主循环 **查表输出**。

→ [Day 7 0x0060 扫描码](../day-07-fifo-mouse/)

---
