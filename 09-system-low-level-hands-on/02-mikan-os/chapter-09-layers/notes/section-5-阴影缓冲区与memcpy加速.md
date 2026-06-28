## 5. 阴影缓冲区与 memcpy 加速

---

### 一、瓶颈在哪

**全屏重绘慢的原因：**

| 操作 | 次数级 |
|------|--------|
| 每像素 **格式判断 / WritePixel** | **宽×高 ≈ 200 万+** |
| 函数调用 + 分支 | 无法 SIMD/块拷贝 |

**关键洞察：** Window 内容已是 **与帧缓冲相同的像素格式** — 合成时只需 **块内存复制**，不必逐点转换。

---

### 二、Shadow Buffer

为 **Window** 增加 **与 GOP Framebuffer 格式一致** 的缓冲：

```
Window 内绘图 → 写入 shadow_buffer_（本地坐标）
合成到屏幕   → memcpy 整块到 FB 的 (pos_x, pos_y) 区域
```

| 对比 | 逐像素 Write | Shadow + memcpy |
|------|--------------|-----------------|
| CPU | 每点函数+分支 | **高度优化 memmove** |
| 书中提升 | baseline | 鼠标重绘 **~67×** |

```cpp
void Window::DrawTo(PixelWriter& screen, Vector2D pos) {
    memcpy(screen_buf_at(pos), shadow_buffer_, width * height * bytes_per_pixel);
}
```

→ [Ch5 Console memcpy 滚动](../chapter-05-console-text/notes/section-5-Console与Newlib.md) — 同类 **块操作** 思想

---

### 三、仍可能全屏合成

**LayerManager::DrawAll** 仍可能 **从底到顶扫全屏** — 但每层 **blit 变为 memcpy**，总成本 **数量级下降**。

**后续可再优化（概念）：** 仅 **脏矩形** 重绘 — 本书 Ch9 以 **全屏+memcpy** 为里程碑。

---

← [4. APIC 测量](./section-4-Local-APIC定时器测量.md) · 下一节 [6. 滚动优化](./section-6-控制台滚动优化与小结.md)
