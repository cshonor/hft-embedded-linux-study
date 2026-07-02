## 2. F2 新终端与光标独立

---

### 一、F2 生成新终端

**全局快捷键 F2：**

```
KeyDown(F2):
    SpawnTask(TaskTerminal);   // 新终端任务 + 新 Terminal 窗口
    Activate(new_layer);
```

| 效果 | 说明 |
|------|------|
| **多个 `>` 提示符** | 各终端 **独立 linebuf / 历史** |
| **各跑各的应用** | **CallApp** 在 **对应 TaskTerminal** 内 |

→ [Ch15 TaskTerminal](../chapter-15-terminal/) · [Ch13 TaskManager](../chapter-13-multitask1/)

---

### 二、光标闪烁重构

**Ch15：** 光标可能由 **Main 统一 tick** — 多终端时 **齐闪/乱闪**。

**Ch24：** **终端任务自持** 闪烁定时：

```cpp
void TaskTerminal::Run() {
    while (true) {
        auto msg = WaitMessage(timeout_for_cursor);
        if (msg == kCursorTick) ToggleCursor();
        …
    }
}
```

| 规则 | 说明 |
|------|------|
| **仅活动终端闪** | 收 **kWindowActive(true)** 才 **EnableCursorBlink** |
| **失活** | **kWindowActive(false)** — **隐藏光标** · 停 tick |

→ [Ch15 活动窗](../chapter-15-terminal/notes/section-3-ActiveLayer与活动窗口.md)

---

### 三、kWindowActive 消息

**点击/Activate 切换焦点时：**

```
Main → old_terminal: kWindowActive(false)
Main → new_terminal: kWindowActive(true)
```

**解决：** 多窗 **只有一个闪烁光标** — **焦点可视化正确**。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. PML4](./section-3-每应用PML4与CR3切换.md)
