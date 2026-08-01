## ③ 命令行 API · 外部 type（EDX 26）

内核 **`type` 命令** 硬编码 — 要 **完全迁出** 需知 **用户输入整行**。

| EDX | 功能 |
|-----|------|
| **26** | **获取命令行字符串** |

**`type.hrb` 流程：**

```
api_getcmdline(buf)
跳过 "type" 与空格 → 提取文件名 → open/read → 显示
```

**Shell 只负责启动 + 传参** — **逻辑全在 .hrb** — 现代 **`/bin/cat` 是用户程序** 的雏形。

→ [Day 20 cmd_app](../day-20-api/) · [Day 26 start/ncst](../day-26-window-speed/)

---
