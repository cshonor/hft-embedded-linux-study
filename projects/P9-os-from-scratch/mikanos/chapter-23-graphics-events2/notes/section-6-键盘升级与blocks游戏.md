## 6. 键盘升级与 blocks 游戏

---

### 一、键盘驱动升级

**Ch12：** 主要 **字符输入**（ASCII）→ TextBox/Terminal。

**Ch23：** 任意键 **按下/松开** — 方向键 · 空格 · 修饰键 **非字符键**。

```cpp
std::bitset<256> prev_keys, curr_keys;

void OnKeyReport(const uint8_t* report) {
    UpdateBitset(curr_keys, report);
    auto pressed  = curr_keys & ~prev_keys;
    auto released = ~curr_keys & prev_keys;
    for (key : pressed)  PostKeyEvent(active_task, kKeyDown, key);
    for (key : released) PostKeyEvent(active_task, kKeyUp, key);
    prev_keys = curr_keys;
}
```

| 技术 | 作用 |
|------|------|
| **USB HID 键码** | 非 ASCII **扫描码** |
| **bitset 边沿** | 与 **鼠标 XOR** 同思路 |

→ [Ch12 USB 键盘](../chapter-12-keyboard/)

---

### 二、Event 类型汇总（本章）

| 类型 | 来源 |
|------|------|
| **kKeyDown / kKeyUp** | 键盘 |
| **kMouseMove** | 鼠标移动 |
| **kMouseButtonDown/Up** | 鼠标键 |
| **kTimeout** | 定时器 |
| **kQuit** | Ctrl+Q（Ch22） |

**统一 ReadEvent 循环** — **现代 GUI 消息泵** 雏形。

---

### 三、blocks 打方块游戏

```cpp
// 游戏状态: 挡板 · 球 · 砖块
while (running) {
    ReadEvent(&ev);
    if (ev.type == kKeyDown) {
        if (ev.key == Left)  paddle.x -= speed;
        if (ev.key == Right) paddle.x += speed;
        if (ev.key == Space) launch_ball();
    }
    if (ev.type == kTimeout) {
        update_physics();
        draw_frame();
        SetTimer(16);
    }
}
```

| 输入 | 行为 |
|------|------|
| **←/→** | 移动 **挡板** |
| **空格** | **发球** |
| **定时器** | **球/碰撞/消砖** |

**集成：** Ch22 **绘制** · Ch23 **全输入** · **定时帧** — **完整小游戏**。

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **鼠标移动/按键** | **eye · paint** |
| **定时器 per-task** | **timer · cube** |
| **键盘 up/down** | **blocks** |
| **事件泵** | **交互体验质变** |

```
Ch23 游戏/动画
    ↓
Ch24 多终端
Ch29 IPC · 多应用协作
```

---

### 五、后续索引

| Ch23 主题 | 继续读 |
|----------|--------|
| 多终端 | [chapter-24-multi-terminal](../chapter-24-multi-terminal/) ⚪ |
| 事件(1) | [chapter-22-graphics-events1](../chapter-22-graphics-events1/) |
| IPC | [chapter-29-ipc](../chapter-29-ipc/) |

---

← [5. cube](./section-5-cube旋转立方体动画.md) · [Ch 22](../chapter-22-graphics-events1/) · [Ch 23 导读](../README.md)
