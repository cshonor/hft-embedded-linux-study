## 3. ActiveLayer 与 ToplevelWindow

---

### 一、活动窗口（Active Window）语义

现代 GUI **三要素：**

| 行为 | 说明 |
|------|------|
| **Z 序置顶** | 点击 → 该 Layer **移到栈顶** |
| **标题栏视觉** | **Active** 与 **Inactive** 不同色 |
| **键盘焦点** | 按键 **只进活动窗** — 非全局 TextBox |

---

### 二、ActiveLayer 类

**管理当前活动 Layer：**

```cpp
class ActiveLayer {
    Layer* active_;
public:
    void Activate(Layer* layer);  // 置顶 + 改标题栏 + 记录 active_
    Layer* GetActive();
};
```

**鼠标点击 hit test：**

```
mousedown on ToplevelWindow → SendMessage(kLayer, Activate, layer_id)
Main → ActiveLayer::Activate → 重绘标题栏
```

→ [Ch10 draggable / hit test](../chapter-10-window/notes/section-6-窗口拖动与draggable.md)

---

### 三、ToplevelWindow 类

**继承 `Window`** — 专管 **带标题栏的顶层窗**：

| 职责 | 说明 |
|------|------|
| **标题栏绘制** | Active/Inactive **配色** |
| **关闭按钮区域** | 点阵或矩形 — 后续扩展 |
| **客户区** | 终端 **Terminal** 内容 |

**键盘路由修改：**

```
Key Message → 若 active_ 是 ToplevelWindow
    → 转发到其 Terminal / 子组件
而非固定全局 TextBox（Ch12）
```

→ [Ch12 键盘](../chapter-12-keyboard/notes/section-5-GUI文本框与退格.md)

---

← [2. kLayer](./section-2-kLayer消息与主任务统一绘制.md) · 下一节 [4. 终端 Task](./section-4-TaskTerminal与Terminal.md)
