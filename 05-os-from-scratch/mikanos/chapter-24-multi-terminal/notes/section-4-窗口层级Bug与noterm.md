## 4. 窗口层级 Bug 与 noterm

---

### 一、ActiveLayer 层级 Bug

**多终端后新建窗置顶逻辑错误** — **重叠显示异常**。

**修复 `ActiveLayer::Activate()`：**

```cpp
void Activate(Layer* layer) {
    layer->SetHeight(0);        // 先置最底层
    BringToTop(layer);          // 再正确置顶
    SendKWindowActive(layer);
}
```

| 原 Bug | 修复 |
|--------|------|
| 直接置顶 **高度计算错** | **先 h=0** 再 **BringToTop** — **Z 序正确** |

→ [Ch9 Layer 高度](../chapter-09-layers/) · [Ch15 ActiveLayer](../chapter-15-terminal/)

---

### 二、noterm — 无终端后台启动

**问题：** 满屏 **黑色 Terminal** — **cube/blocks** 窗口被挡。

**内置命令 `noterm`：**

```
> noterm cube
```

| 步骤 | 行为 |
|------|------|
| 1 | 创建 **TaskTerminal** · **`show_window = false`** |
| 2 | **不绘制** 终端 Layer（或 **零尺寸/隐藏**） |
| 3 | **自动注入** 命令行 `"cube"` 到该任务 **linebuf** |
| 4 | 等价用户在该 **隐藏终端** 敲 **cube** |

**效果：** **仅应用窗口** 可见 — **桌面更清爽**。

---

### 三、实现要点

```cpp
if (strcmp(cmd, "noterm") == 0) {
    auto* term_task = SpawnHiddenTerminal();
    term_task->InjectLine(args);   // "cube" / "blocks"
    term_task->RunOneCommand();
}
```

**仍走 **CallApp + 专属 PML4**** — 只是 **无 Terminal UI**。

---

← [3. PML4](./section-3-每应用PML4与CR3切换.md) · 下一节 [5. KillApp](./section-5-用户态异常与KillApp.md)
