## 5. 后置缓冲区（Back Buffer）

---

### 一、仍存的闪烁：鼠标 vs 计数器

**局部重绘后：** 计数器更新与 **鼠标层** 绘制存在 **极短时间差**：

```
时刻 T1: 计数器 blit 到 FB
时刻 T2: 鼠标   blit 到 FB   ← 用户可能看到中间态
```

光标在 **快速刷新区域** 上时 → **光标闪烁**。

---

### 二、双缓冲策略

引入 **Back Buffer** — 与物理 FB **同尺寸、同像素格式** 的 **隐藏缓冲**：

```
DrawAllToBackBuffer():
    清空 back_buffer
    for layer in layer_stack_ (bottom → top):
        composite layer into back_buffer

Flip / Present:
    memcpy(physical_framebuffer, back_buffer, full_size);
    // 或一次 bulk copy — 用户只见到最终帧
```

| 对比 | 直接写 FB | Back Buffer |
|------|-----------|-------------|
| 合成过程 | **可见** 中间层 | **不可见** — 离屏完成 |
| 闪烁 | 层间竞态 | **单帧呈现** — 彻底消除 |

**术语：** Shadow Buffer（Ch9）侧重 **每层离屏**；Back Buffer 侧重 **整屏合成缓冲** — 二者可 **并存**。

---

### 三、性能权衡

| 成本 | 说明 |
|------|------|
| **额外一整屏 RAM** | Ch8 **Allocate** 4KiB 对齐缓冲 |
| **每帧一次全屏 memcpy** | Present 仍 O(像素) — 但 **无重复 partial 闪** |

**动态 UI 标准解法：** 游戏/GUI 普遍 **double buffering** 或 **triple buffering**。

---

← [4. 局部重绘](./section-4-局部重绘与矩形交集.md) · 下一节 [6. 拖动](./section-6-窗口拖动与draggable.md)
