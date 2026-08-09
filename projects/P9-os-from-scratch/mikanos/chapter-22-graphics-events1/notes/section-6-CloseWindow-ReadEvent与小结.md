## 6. CloseWindow、ReadEvent 与小结

---

### 一、CloseWindow 与 RemoveLayer

**Ch21 问题：** **exit 后窗口仍留屏**。

```cpp
// LayerManager
void RemoveLayer(Layer* layer) {
    DetachFromStack(layer);
    FreeShadowBuffer(layer);
    RequestFullRedrawBelow();
}

int64_t SyscallCloseWindow(LayerId id) {
    VerifyOwner(CurrentTask(), id);
    RemoveLayer(GetLayer(id));
    return 0;
}
```

| 步骤 | 效果 |
|------|------|
| **RemoveLayer** | 从 **图层栈** 摘除 |
| **背景重绘** | 露出 **下层桌面/窗口** |
| **应用 exit 前调用** | **无残骸** |

→ [Ch9 LayerManager](../chapter-09-layers/)

---

### 二、ReadEvent — 事件驱动

```cpp
struct Event { enum Type { kKey, kQuit, … }; … };

int64_t SyscallReadEvent(Event* out) {
    Task* t = CurrentTask();
    t->SetWaiting();                    // Ch14 Sleep
    // 队列空则 schedule 出去
    *out = t->PopEvent();               // 键盘中断/Main 投递
    return 0;
}
```

| 行为 | 说明 |
|------|------|
| **无事件** | 任务 **Sleep** — **不占 CPU**（Idle+hlt） |
| **有事件** | **唤醒** · 拷贝到 **用户 Event 缓冲** |
| **winhello 改造** | 循环 **ReadEvent** 直到 **kQuit (Ctrl+Q)** |

→ [Ch14 Sleep/Wakeup](../chapter-14-multitask2/) · [Ch12 键盘](../chapter-12-keyboard/)

---

### 三、syscall 期间栈 #PF 修复

**开发 ReadEvent 时发现：** **syscall 路径** 栈切换不当 → **#PF**。

| 修复方向 | 与 Ch21 **IST** / Ch20 **TSS** 同类 |
|----------|--------------------------------------|
| **内核 syscall 栈** | 保证 **SyscallEntry → handler** 始终 **有效内核 RSP** |
| **返回用户** | **sysret** 恢复 **用户 RSP** 在 **User 映射** 内 |

**保证：** **阻塞等事件** 时 **反复 syscall** 仍稳定。

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **exit/atexit** | 标准 C 退出 |
| **FillRect · DrawLine · DoWinFunc** | **stars · lines** |
| **NO_REDRAW + WinRedraw** | **~99×** 绘图加速 |
| **CloseWindow** | 干净关窗 |
| **ReadEvent** | **事件驱动** · **Ctrl+Q** |

```
Ch22 键盘事件 + 绘制优化
    ↓
Ch23 图形和事件(2)
Ch29 IPC
```

---

### 五、后续索引

| Ch22 主题 | 继续读 |
|----------|--------|
| 事件(2) | [chapter-23-graphics-events2](../chapter-23-graphics-events2/) ⚪ |
| 窗口 syscall | [chapter-21-window-apps](../chapter-21-window-apps/) |
| 多任务等待 | [chapter-14-multitask2](../chapter-14-multitask2/) 🔴 |

---

← [5. DrawLine](./section-5-WinDrawLine与lines命令.md) · [Ch 21](../chapter-21-window-apps/) · [Ch 22 导读](../README.md)
