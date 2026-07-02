## 1. 本章定位

> **《从零自制操作系统》Ch 21 窗口应用**

---

### 一、从 Ch20 到「完整应用」

| Ch 20 | **Ch 21** |
|-------|-----------|
| **单一 syscall** — 终端字符串 | **PutString / exit / OpenWindow …** |
| rpn **过程打印** 验证 | **`printf`** · **winhello GUI** |
| 应用 **死循环结束** | **`exit` 回到 OS 事件循环** |

**转折：** 应用具备 **标准库 I/O · 优雅退出 · 独立窗口** — 脱离 **纯 CLI**。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **IST1** | 定时器中断 **专用内核栈** |
| **PutString + write()** | **任务 ID → 终端** · **Newlib** |
| **exit (0x80000002)** | **CallApp 栈/寄存器恢复** |
| **syscall.h** | 应用侧 **统一声明** |
| **OpenWindow / WinWriteString** | **图层 ID** · 坐标 · 颜色 |
| **winhello** | 三行三色 **hello world!** |

---

### 三、后续章节

```
Ch21 窗口 syscall + winhello  ← 本章
    ↓
Ch22+ 图形事件 · 鼠标键盘到应用
Ch29 IPC
```

→ [Ch20 系统调用](../chapter-20-syscall/)

---

← [Ch 21 导读](../README.md) · 下一节 [2. IST](./section-2-IST与定时器中断栈修复.md)
