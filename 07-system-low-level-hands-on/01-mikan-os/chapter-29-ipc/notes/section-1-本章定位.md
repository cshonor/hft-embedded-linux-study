## 1. 本章定位

> **《从零自制操作系统》Ch 29 应用间通信**

---

### 一、从 Ch28 重定向到 IPC

| Ch 28 | **Ch 29** |
|-------|-----------|
| **`>`** — stdout → **文件** | **`\|`** — stdout → **下一命令 stdin** |
| 单命令 **输出定向** | **多命令协同 · 流式处理** |
| **PrintToFD** | **PipeDescriptor + 双 Task 并发** |

**跨越：** 单进程 I/O → **进程间字节流/共享内存** — **Shell 管道** 落地。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **`echo $?`** | **last_exit_code_** |
| **管道 `\|`** | **cat memmap \| grep hoge** |
| **PipeDescriptor** | **kPipe 消息 · 隐藏终端** |
| **WaitFinish/Finish** | **等右端结束** |
| **sort** | **行排序 · 组合 grep** |
| **共享内存** | **同 PFN 多映射 · 数据竞争** |

---

### 三、后续

```
Ch29 IPC  ← 本章
    ↓
Ch30 额外应用 · 附录
```

→ [Ch28 重定向](../chapter-28-japanese-redirect/)

---

← [Ch 29 导读](../README.md) · 下一节 [2. $?](./section-2-退出码与echo-question.md)
