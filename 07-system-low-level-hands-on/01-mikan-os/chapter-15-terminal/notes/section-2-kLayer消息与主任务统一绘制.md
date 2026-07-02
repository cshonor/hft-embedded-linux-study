## 2. kLayer 消息与主任务统一绘制

---

### 一、残影 Bug 根因

**现象：** 用鼠标拖动 **TaskB 窗口** 时 **留下残影**、破坏画面。

| 冲突方 | 操作 |
|--------|------|
| **TaskB** | 更新 **自己 Layer** 位置/内容 |
| **Main Task** | 移动 **鼠标 Layer** · 合成 **Back Buffer** |

**同时写底层缓冲/合成状态** → **数据竞争** — 与 Ch7 队列竞态 **同构**，在 **图形层** 暴露。

→ [Ch14 主任务 Level 3](../chapter-14-multitask2/notes/section-5-任务优先级Level.md)

---

### 二、消息传递：kLayer

**原则：** **仅 Main Task** 执行 **LayerManager** 变更与 **最终 Draw**。

**后台 TaskB 不再直接：**

```cpp
layer->MoveTo(x, y);   // ❌ 并发冲突
```

**改为 SendMessage 到 Main：**

```cpp
Message m { .type = kLayer, .op = MoveLayer, .layer_id, .pos };
SendMessage(main_task, m);
```

**Main 处理 kLayer：**

```
Pop kLayer → LayerManager::Move / Activate / … → Draw(back_buffer)
```

| 收益 | 说明 |
|------|------|
| **单写者** | 合成 **串行化** — 无残影 |
| **性能** | Main 已 **最高优先级** + 可 **局部 Draw** |

---

### 三、设计模式

```
后台 Task：  逻辑状态 + 发 kLayer 请求
Main Task：  唯一 GPU/FB「驱动线程」— 合成器
```

→ 类似 **Wayland compositor** / **Windows DWM** 的 **缩小版**

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 活动窗口](./section-3-ActiveLayer与ToplevelWindow.md)
