## 3. 像素格式与 WritePixel

---

### 一、任意坐标绘色的前提

Ch 3 已能写帧缓冲；本章要在 **(x, y)** 写入 **指定 RGB 颜色**。

| GOP 报告项 | 绘图需要 |
|------------|----------|
| **FrameBufferBase** | 像素数组起点 |
| **HorizontalResolution / VerticalResolution** | 边界检查 |
| **PixelFormat** | **字节顺序** — RGB 还是 BGR |

**常见 32-bit 格式（示意）：**

| 格式 | 内存字节顺序（低→高） |
|------|------------------------|
| **BGR** | B, G, R, X（保留） — UEFI 常见 |
| **RGB** | R, G, B, X |

同一 `0xFF0000` 语义（「红」）在不同格式下 **写入字节不同**。

---

### 二、初级 `WritePixel()` 实现

```cpp
void WritePixel(PixelFormat fmt, uint8_t* fb, int x, int y, uint32_t rgb) {
    // 根据 fmt 分支：写 BGR 或 RGB 分量到 fb[...]
    if (fmt == PixelFormat::BGR) { /* ... */ }
    else if (fmt == PixelFormat::RGB) { /* ... */ }
}
```

**全屏双重循环：**

```
for y … for x …
    WritePixel(fmt, …);   // 每次调用都判断 fmt
```

| 问题 | 数量级 |
|------|--------|
| 1920×1080 | **~200 万次** 相同分支 |
| **分支在热路径** | 预测虽可学，但仍多余 · 阻碍内联优化 |

→ 与 HFT「热路径少分支」同构 — 下文用 **多态一次分派** 解决。

---

### 三、坐标与 stride

实际写入偏移常需 **bytes_per_pixel** 与 **pitch（stride）**：

```
offset = y * pitch + x * bytes_per_pixel
```

GOP `Mode->Info` 提供 **PixelsPerScanLine** — 可能 **≥ width**（对齐填充）。

→ 衔接 [Ch3 GOP](../chapter-03-bootloader-display/notes/section-4-GOP与帧缓冲区.md)

---

← [2. make](./section-2-make与Makefile.md) · 下一节 [4. PixelWriter](./section-4-PixelWriter与vtable.md)
