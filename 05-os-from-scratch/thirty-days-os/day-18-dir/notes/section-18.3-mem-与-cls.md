## ③ mem 与 cls · 引入 strcmp

#### 为何 `strcmp`

命令变多 → **逐字符 if 链** 易错 → **`#include <string.h>`**：

```c
if (strcmp(cmd, "mem") == 0) { … }
else if (strcmp(cmd, "cls") == 0) { … }
else if (strcmp(cmd, "dir") == 0) { … }
```

#### `mem` 命令

| 之前 | 之后 |
|------|------|
| 桌面 **硬编码** 显示内存 | 输入 **`mem`** **按需查询** |

创建 console 时传入 **`memtotal` / `memman` 指针**（Day 15 栈传参同类技巧）→ 打印 **总内存 / 可用内存**。

→ [Day 9 MEMMAN](../day-09-memory/)

#### `cls` 命令

**clear screen：** **黑色矩形** 涂满 Console 文字区 + **光标 y 回顶**。

---
