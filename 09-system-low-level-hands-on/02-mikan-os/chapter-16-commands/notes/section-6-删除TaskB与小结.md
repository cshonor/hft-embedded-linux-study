## 6. 删除 TaskB 与小结

---

### 一、为何删除 TaskB

**Ch13–14：** **TaskB** 高速计数 — 验证 **多任务 / Sleep / 优先级**。

| 现状 | 问题 |
|------|------|
| 系统已成熟 | TaskB **无演示价值** |
| 全速计数 | QEMU **CPU 占用高** · **费电** |

**删除 TaskB 后：** CPU 使用率 **降至个位数 %** — **Idle + hlt** 主导。

→ [Ch14 Idle Task](../chapter-14-multitask2/notes/section-6-Idle-Task与小结.md)

---

### 二、本章总结

| 成果 | 说明 |
|------|------|
| **linebuf_ + Scroll1** | 终端 **可编辑输入** |
| **`>` + echo/clear/lspci** | **最小 CLI** |
| **历史 ↑↓** | **deque×8** · **signed/unsigned** 教训 |
| **删 TaskB** | **省电** |

```
Ch15 终端 UI
    ↓
Ch16 可交互命令行  ← 本章
    ↓
Ch17 文件系统 · Ch20 syscall · 用户程序
```

---

### 三、后续索引

| Ch16 主题 | 继续读 |
|----------|--------|
| 文件系统 | [chapter-17-filesystem](../chapter-17-filesystem/) 🟡 |
| 应用 | [chapter-18-apps](../chapter-18-apps/) ⚪ |
| 系统调用 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |
| PCI 基础 | [chapter-06-mouse-pci](../chapter-06-mouse-pci/) |

---

← [5. 历史](./section-5-命令历史与方向键.md) · [Ch 15](../chapter-15-terminal/) · [Ch 16 导读](../README.md)
