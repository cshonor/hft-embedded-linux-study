## 6. 小结与后续

---

### 一、本章总结

| 成果 | 说明 |
|------|------|
| **F2 多终端** | **并行 CallApp** |
| **光标/kWindowActive** | **焦点正确** |
| **每应用 PML4** | **同 VA 多实例** · **CR3 切换** |
| **Activate 修复** | **多窗 Z 序** |
| **noterm** | **隐藏终端 · 清爽 GUI** |
| **KillApp** | **CPL=3 异常 · OS 存活** |

```
Ch24 多终端工作站
    ↓
Ch25 应用读文件
Ch29 IPC
```

---

### 二、设计对照

| 能力 | 现代 OS 对应 |
|------|--------------|
| **多 Terminal** | 多 **TTY / 终端模拟器** |
| **per-process CR3** | **进程 mm** |
| **Kill on user fault** | **信号 SIGSEGV / 杀进程** |
| **noterm** | **daemon / 无 controlling tty** 简化版 |

---

### 三、后续索引

| Ch24 主题 | 继续读 |
|----------|--------|
| 应用读文件 | [chapter-25-app-read-file](../chapter-25-app-read-file/) ⚪ |
| IPC | [chapter-29-ipc](../chapter-29-ipc/) |
| 分页基础 | [chapter-19-paging](../chapter-19-paging/) 🔴 |
| 系统调用 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |

---

← [5. KillApp](./section-5-用户态异常与KillApp.md) · [Ch 23](../chapter-23-graphics-events2/) · [Ch 24 导读](../README.md)
