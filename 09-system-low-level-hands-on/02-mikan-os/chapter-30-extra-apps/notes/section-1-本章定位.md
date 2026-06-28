## 1. 本章定位

> **《从零自制操作系统》Ch 30 额外应用**

---

### 一、主体开发的句号

| Ch 1–29 | **Ch 30** |
|---------|-----------|
| 引导 · 内核 · GUI · syscall · FS · IPC | **实用 app + 体验 polish** |
| `apps/grep` 全路径 | **`grep` 短名** — **FindCommand** |
| × 装饰 | **真关窗** · **tview/gview** |

**定位：** 不是新子系统 — **把已有能力「用起来、好用」**。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **FindCommand** | **apps/** PATH 搜索 |
| **more** | **`ls apps \| more`** · 管道按键 |
| **cat stdin** | **`cat > foobar`** |
| **× 按钮** | **kWindowClose → kQuit** |
| **tview** | 滚动 · **getopt -w -h -t** |
| **gview** | **stb_image** · **64KiB 栈** |

---

### 三、后续

```
Ch30 额外应用  ← 本章（主体末章）
    ↓
Ch31 今后 · 附录
```

→ [Ch29 管道](../chapter-29-ipc/) · [Ch31 今后](../chapter-31-road-ahead/)

---

← [Ch 30 导读](../README.md) · 下一节 [2. PATH](./section-2-FindCommand与PATH搜索.md)
