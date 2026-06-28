## 6. 小结与索引

---

### 一、本章总结

| 成果 | 说明 |
|------|------|
| **kLayer → Main** | **单写者合成** — 无拖窗残影 |
| **ActiveLayer + ToplevelWindow** | **活动窗** · 键盘焦点 |
| **TaskTerminal + Terminal** | 黑底白字 · **独立 Task** |
| **Caret 0.5s** | 定时器 + **DrawArea 7×15** · **~16×** |

**架构图：**

```
TaskTerminal ──kLayer/Redraw──► Main Task ──► LayerManager / Back Buffer
Keyboard ──► Active ToplevelWindow ──► TaskTerminal
```

---

### 二、Ch16 铺垫

| 已有 | 下一章 |
|------|--------|
| 终端 **显示** + **焦点** | **命令解析 · 执行** |
| 键盘进 **活动 Terminal** | **shell 内建命令** |

→ [chapter-16-commands](../chapter-16-commands/)

---

### 三、后续索引

| Ch15 主题 | 继续读 |
|----------|--------|
| 命令 shell | [chapter-16-commands](../chapter-16-commands/) ⚪ |
| 多终端 | [chapter-24-multi-terminal](../chapter-24-multi-terminal/) ⚪ |
| 图层基础 | [chapter-09-layers](../chapter-09-layers/) |
| 调度/消息 | [chapter-14-multitask2](../chapter-14-multitask2/) |

---

← [5. DrawArea](./section-5-DrawArea局部重绘.md) · [Ch 14](../chapter-14-multitask2/) · [Ch 15 导读](../README.md)
