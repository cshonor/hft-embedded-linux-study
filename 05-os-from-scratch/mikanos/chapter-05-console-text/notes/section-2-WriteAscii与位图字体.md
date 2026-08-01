## 2. WriteAscii 与位图字体

---

### 一、字体 = 形状数据

**字体：** 用数据描述 **字符长什么样** — 非魔法，就是 **像素图案**。

| 本章初版 | 说明 |
|----------|------|
| **1 bit / 像素** | 0 = 不上色 · 1 = 用前景色画点 |
| **二维数组** | 行 × 列 — 如 8×16 点阵 |
| **ASCII 码** | 索引到对应字模 |

```
字模 'A' 示意（每行 8 bit）:
  0b00111100
  0b01100110
  ...
```

---

### 二、`WriteAscii()` 实现思路

```cpp
void WriteAscii(PixelWriter* writer, int x, int y, char c, PixelColor fg) {
    const uint8_t* glyph = font_data[c];  // 指向该字符位图
    for (int dy = 0; dy < char_height; ++dy) {
        uint8_t row = glyph[dy];
        for (int dx = 0; dx < char_width; ++dx) {
            if (row & (0x80 >> dx))   // 位运算：该点是否置 1
                writer->Write(x + dx, y + dy, fg);
        }
    }
}
```

| 技术点 | 说明 |
|--------|------|
| **`0x80 >> dx`** | 从高位逐 bit 检测 |
| **调用 PixelWriter** | 复用 Ch4 **BGR/RGB** 抽象 |
| **坐标** | `(x,y)` 为字符左上角 |

→ [Ch4 像素格式](../chapter-04-pixel-make/notes/section-3-像素格式与WritePixel.md)

---

### 三、从单字符到字符串

```
WriteAscii × N  →  WriteString（递增 x，遇 \0 停止）
```

**局限：** 初版字体仅 **少量 ASCII** —  richer 字符见 [4. 東雲嵌入](./section-4-外部字体嵌入.md)。

→ [appendix-F ASCII 表](../../appendix-F-ascii-table/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 代码拆分](./section-3-代码拆分与make.md)
