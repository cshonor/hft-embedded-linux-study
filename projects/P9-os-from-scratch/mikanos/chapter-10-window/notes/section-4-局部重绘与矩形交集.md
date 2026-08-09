## 4. 局部重绘与矩形交集

---

### 一、重载 `LayerManager::Draw()`

**不再每次 `DrawAll()`** — 支持：

| 重载（概念） | 作用 |
|--------------|------|
| **`Draw(layer_id)`** | 只合成 **指定图层** |
| **`Draw(Rectangle area)`** | 只更新 **屏幕矩形区域** |

**计数器更新时：**

```
仅 mark 窗口 Layer 的客户区 或 文字包围盒
Draw(dirty_rect)  // 而非全屏
```

| 收益 | 说明 |
|------|------|
| **memcpy 量 ↓** | 只 touch 脏像素 |
| **闪烁减轻** | 仍可能有 **鼠标 vs 动态区** 竞态（§5 解决） |

---

### 二、矩形交集 — 重载 `operator&`

层 blit 到屏幕时，层 **屏幕坐标矩形** 与 **脏区矩形** 求交：

```cpp
Rectangle intersection = layer_bounds & dirty_area;
if (intersection.IsEmpty()) return;
memcpy(dest + intersection.origin, src + ..., intersection.size);
```

| `Rectangle &` | 含义 |
|---------------|------|
| **交集** | 仅复制 **两层都覆盖** 的像素 |
| **空矩形** | 跳过 — 无 wasted work |

**确保：** 局部 Draw 时 **内存复制边界正确** — 不越界、不扫全屏。

---

### 三、与 Ch9 Shadow Buffer 关系

局部 Draw 仍可在 **Window shadow** 上更新 — 合成阶段 **只 blit 交集** 到 Back Buffer 或 FB。

→ [Ch9 Shadow](../chapter-09-layers/notes/section-5-阴影缓冲区与memcpy加速.md)

---

← [3. 计数器与闪烁](./section-3-计数器与闪烁问题.md) · 下一节 [5. Back Buffer](./section-5-后置缓冲区.md)
