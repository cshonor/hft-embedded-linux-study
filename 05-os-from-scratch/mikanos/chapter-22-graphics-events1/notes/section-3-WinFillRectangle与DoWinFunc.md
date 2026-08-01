## 3. WinFillRectangle 与 DoWinFunc

---

### 一、WinFillRectangle 系统调用

**在窗口客户区绘制实心矩形（或 1×1 像素点）：**

```cpp
WinFillRectangle(layer_id, x, y, w, h, color);
// w=h=1 → 单点 — stars 用
```

| 参数 | 内核 |
|------|------|
| **layer_id** | Ch21 **OpenWindow** 返回 |
| **x,y,w,h,color** | 写 **Layer 阴影缓冲** · 触发重绘（可 defer） |

→ [Ch10 窗口客户区](../chapter-10-window/) · [Ch9 阴影缓冲](../chapter-09-layers/)

---

### 二、DoWinFunc() — 可变参模板

**syscall 增多 → 重复 **「取 layer · 校验 · 调 Writer · Redraw」：**

```cpp
template<typename... Args>
int64_t DoWinFunc(int64_t id, void (*f)(Window&, Args...), Args... args) {
    auto* win = GetWindowByLayerId(CurrentTask(), id);
    f(*win, args...);
    MaybeRedraw(id);
    return 0;
}
```

| 收益 | 说明 |
|------|------|
| **一份校验逻辑** | **任务归属** · **无效 ID** |
| **多 syscall 复用** | FillRect · DrawLine · WriteString … |
| **C++ 模板** | 编译期展开 — **零运行时虚表开销** |

---

### 三、stars 命令

```cpp
auto lid = OpenWindow(800, 600, "stars");
for (int i = 0; i < N; ++i) {
    int x = rand() % w, y = rand() % h;
    WinFillRectangle(lid | LAYER_NO_REDRAW, x, y, 1, 1, WHITE);
}
WinRedraw(lid);
ReadEvent(...);   // 等退出
```

**效果：** 黑色夜空 **随机白点** — 验证 **批量像素 API**。

---

← [2. exit/atexit](./section-2-exit规范化与atexit.md) · 下一节 [4. 重绘优化](./section-4-性能测量与批量重绘.md)
