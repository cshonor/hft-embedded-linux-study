## 3. FreeType 与矢量字体

---

### 一、从点阵到 TrueType

**Ch5：** **hankaku.bin** 等 **固定位图** — 仅 **ASCII 级**。

**Ch28：** 链接 **FreeType** — 解析 **`.ttf`**（如 **`nihongo.ttf`**）— **任意码位 glyph**。

```cpp
FT_InitFreeType(&library);
FT_New_Face(library, "nihongo.ttf", 0, &face);
FT_Set_Pixel_Sizes(face, 0, 16);
FT_Load_Char(face, codepoint, FT_LOAD_RENDER);
// face->glyph->bitmap → blit to shadow buffer
```

| 步骤 | 说明 |
|------|------|
| **Load_Char** | Unicode → **glyph 轮廓** |
| **FT_Render_Glyph** | **栅格化** 为 bitmap |
| **Draw** | 写入 **Terminal/Window** 像素 |

---

### 二、字体文件与引导读卷

**nihongo.ttf ≈ 6MiB** — 需 **FAT 卷内可读**。

**Ch17 引导 Block I/O 预读：** **16MiB → 32MiB**

```
Bootloader ReadBlocks(0, 32MiB, buffer)
```

| 原因 | 说明 |
|------|------|
| **字体 + apps + kernel 镜像** | 16MiB **不够** |
| **仍内存 FS** | 真机 **Boot Services 退出前** 读完 |

→ [Ch17 Block I/O 预读](../chapter-17-filesystem/notes/section-5-UEFI-Block-IO与卷镜像.md)

---

### 三、内核链接 FreeType

**构建：** 交叉编译 **FreeType 静态库** · 内核 Makefile **LDFLAGS += -lfreetype**。

**体积/栈：** 复杂库 — 引发 §4 **栈扩容**。

→ [Ch5 外部字体对比](../chapter-05-console-text/notes/section-4-外部字体嵌入.md)

---

← [2. UTF-8](./section-2-UTF-8解析与排版.md) · 下一节 [4. 栈扩容](./section-4-栈扩容与FreeType-Bug.md)
