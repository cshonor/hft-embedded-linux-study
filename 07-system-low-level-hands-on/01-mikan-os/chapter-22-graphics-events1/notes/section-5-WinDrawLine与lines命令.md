## 5. WinDrawLine 与 lines 命令

---

### 一、WinDrawLine 系统调用

```cpp
WinDrawLine(layer_id, x0, y0, x1, y1, color);
```

**内核算法（概念）：**

```
dx = x1 - x0,  dy = y1 - y0
若 |dx| >= |dy|:  以 x 步进，y = y0 + dy/dx * (x - x0)
否则:            以 y 步进，x = …
```

| 细节 | 说明 |
|------|------|
| **斜率方向** | 选 **floor** 或 **ceil** 避免 **断点/空洞** |
| **实现** | Bresenham 或 **浮点步进**（本书结合 **libm**） |

**DoWinFunc** 同样包装 **权限 + 重绘标志**。

---

### 二、链接 libm

```makefile
APP_LDFLAGS += -lm
```

```cpp
#include <math.h>
double angle = 2 * M_PI * i / N;
int x = cx + (int)(cos(angle) * r);
int y = cy + (int)(sin(angle) * r);
WinDrawLine(lid | NO_REDRAW, cx, cy, x, y, palette[i]);
```

**lines 命令：** 从中心 **放射 N 条彩色直线** — 验证 **三角函数 + 直线 API**。

---

### 三、与 stars 相同优化

```cpp
for (…) WinDrawLine(lid | NO_REDRAW, …);
WinRedraw(lid);
```

**大批量线段** 亦需 **defer redraw** — 否则 **合成瓶颈** 再现。

---

← [4. 重绘优化](./section-4-性能测量与批量重绘.md) · 下一节 [6. ReadEvent](./section-6-CloseWindow-ReadEvent与小结.md)
