## 6. 控制台滚动优化与小结

---

### 一、Console 滚动瓶颈

Ch5 **Console::Scroll** 在 **文本逻辑** 上 `memcpy` 行 — 若实现为 **逐字重绘全屏文字**：

| 问题 | 后果 |
|------|------|
| 滚动一次 **重画所有字符** | 与鼠标问题 **同构** — 极慢 |

---

### 二、像素级块移动优化

在 **Layer/Window** 框架下，控制台是一层 **Window**：

```
Scroll():
    1. memcpy：Window 阴影缓冲内「第 2 行~末行」→ 上移一行
    2. 仅 **清空并重绘最底一行** 文字
    3. LayerManager 合成 — 该层 blit 用 memcpy
```

| 对比 | 全屏重绘所有字 | **上移像素 + 只画末行** |
|------|----------------|-------------------------|
| 书中提升 | baseline | **~11×** |

**与鼠标优化共性：** **少做重复工作** · **memcpy 块操作** · **测量验证**

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **`sbrk`/`new`** | 动态 Layer/Window |
| **LayerManager** | **Z 序合成** — 不破坏底图 |
| **APIC 计时** | baseline **~2.5×10⁸** |
| **Shadow Buffer** | 鼠标 **~67×** |
| **滚动优化** | Console **~11×** |

**方法论：**

```
正确性（图层）→ 测量（APIC）→ 瓶颈（逐像素）→ 优化（memcpy）→ 再测量
```

---

### 四、后续索引

| Ch9 主题 | 继续读 |
|----------|--------|
| 窗口系统 | [chapter-10-window](../chapter-10-window/) ⚪ |
| APIC/定时器 | [chapter-11-timer-acpi](../chapter-11-timer-acpi/) 🔴 |
| 内存 | [chapter-08-memory](../chapter-08-memory/) |
| SysPerf | [03-Systems-Performance-2nd](../../../03-Systems-Performance-2nd/) |

---

← [5. Shadow Buffer](./section-5-阴影缓冲区与memcpy加速.md) · [Ch 8](../chapter-08-memory/) · [Ch 9 导读](../README.md)
