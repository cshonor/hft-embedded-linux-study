## 5. 窗口关闭按钮

---

### 一、从装饰到功能

**Ch10–21：** 标题栏 **×** 仅 **绘制** — 点击 **无效果**。

**Ch30：** **鼠标左键抬起** 在 **关闭区** → **关窗流程**。

---

### 二、GetWindowRegion()

```cpp
enum WindowRegion { kClient, kTitleBar, kCloseButton, kBorder, … };

WindowRegion GetWindowRegion(Window& w, Vector2D pos) {
    if (close_button_rect.Contains(pos))
        return kCloseButton;
    …
}
```

**Main 鼠标处理：**

```cpp
if (region == kCloseButton && button_up) {
    PostMessage(window_task, Message::kWindowClose);
}
```

| 设计 | **不** 内核 **强行 RemoveLayer** |
|------|-----------------------------------|
| **消息** | 应用 **ReadEvent** 收 **kQuit** · **CloseWindow · exit** |

→ [Ch22 CloseWindow](../chapter-22-graphics-events1/notes/section-6-CloseWindow-ReadEvent与小结.md) · [Ch23 鼠标按键](../chapter-23-graphics-events2/notes/section-3-鼠标按键与paint绘图.md)

---

### 三、安全关闭链

```
用户点 ×
  → kWindowClose
  → Task 转 kQuit Event
  → app: CloseWindow(); exit(0);
  → CleanPageMaps · RemoveLayer
```

**与 KillApp 对比：** **协作式退出** — **资源 **应用** 可释放**（atexit 等）。

→ [Ch21 exit](../chapter-21-window-apps/notes/section-4-exit系统调用与CallApp栈恢复.md)

---

← [4. cat](./section-4-cat标准输入与重定向建文件.md) · 下一节 [6. tview/gview](./section-6-tview-gview与小结.md)
