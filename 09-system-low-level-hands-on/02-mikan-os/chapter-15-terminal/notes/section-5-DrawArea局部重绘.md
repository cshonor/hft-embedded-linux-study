## 5. DrawArea 局部重绘

---

### 一、问题：光标闪 = 整窗重绘

初版 **caret 切换** 触发 **整个 Terminal Layer** 合成：

| 成本 | 量级 |
|------|------|
| 终端客户区 | 可能 **数万~百万像素** |
| 频率 | **2Hz**（0.5s）— 仍浪费 |

**与 Ch10 计数器闪烁** 同构 — 需 **脏矩形**。

→ [Ch10 局部 Draw](../chapter-10-window/notes/section-4-局部重绘与矩形交集.md)

---

### 二、LayerManager::Draw(DrawArea)

**新增限定范围重绘 API：**

```cpp
layer_manager.Draw(/* layer_id, */ Rectangle{ x, y, 7, 15 });
// 光标区域约 7×15 像素（书中尺寸）
```

| 步骤 | 说明 |
|------|------|
| 计算 **caret 包围盒** | 7 宽 × 15 高 |
| 仅 **该矩形** 与 Layer 求交 | `operator&` |
| Back Buffer **局部** 合成 → Present 或 **局部 copy 到 FB** |

---

### 三、性能结果（书中）

| 对比 | 全终端重绘 | **DrawArea 7×15** |
|------|------------|-------------------|
| 相对速度 | 1× | **~16×** |

**原则：** **最小 inval 区域** — GUI 性能 **基本功**。

→ [Ch9 测量方法论](../chapter-09-layers/notes/section-4-Local-APIC定时器测量.md)

---

← [4. 终端 Task](./section-4-TaskTerminal与Terminal.md) · 下一节 [6. 小结](./section-6-小结与索引.md)
