## 6. tview、gview 与小结

---

### 一、tview 文本查看器

**比 cat/more 更舒适 — GUI 滚动：**

```cpp
// getopt 解析
while ((c = getopt(argc, argv, "w:h:t:")) != -1) {
    switch (c) {
    case 'w': win_w = atoi(optarg); break;
    case 'h': win_h = atoi(optarg); break;
    case 't': tab_width = atoi(optarg); break;
    }
}
```

| 输入 | 行为 |
|------|------|
| **↑↓** | 行滚动 |
| **PageUp/Down** | 页滚动 |
| **文件参数** | **MapFile/open** 读入 · **Wrap/Tab 渲染** |

**依赖：** Ch22 **ReadEvent** · Ch21 **OpenWindow/WinWriteString** · Ch25 **读文件**。

→ [Ch18 getopt 需 Newlib](../chapter-18-apps/notes/section-6-C++应用标准库与小结.md)

---

### 二、gview 图像查看器

**stb_image — JPEG/BMP 解码：**

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
unsigned char* pixels = stbi_load(path, &w, &h, &comp, 4);
// WinFillRectangle / 写 shadow 缓冲逐像素
```

| 条件 | 说明 |
|------|------|
| **x86-64 SSE2** | 浮点/ SIMD — **解码库友好** |
| **栈 64KiB** | Ch28 **32KiB** 仍不够 — **unknown marker** 等 **栈溢出** |

```cpp
constexpr size_t kAppStackBytes = 64 * 1024;
```

**与 FreeType 同类教训：** **第三方库 → 测栈**。

→ [Ch28 栈 32KiB](../chapter-28-japanese-redirect/notes/section-4-栈扩容与FreeType-Bug.md)

---

### 三、全书主体总结（Ch1–30）

| 层 | 里程碑 |
|----|--------|
| **引导** | UEFI · ELF · GOP |
| **内核** | 中断 · 分页 · 多任务 |
| **GUI** | Layer · 窗口 · 事件 |
| **用户态** | syscall · fd · 管道 |
| **体验** | PATH · more · tview · gview |

**MikanOS：** **可启动 · 可交互 · 可跑 app · 可读写盘 · 可管道协作** 的 **64 位 OS 雏形**。

---

### 四、后续索引

| 主题 | 继续读 |
|------|--------|
| 今后展望 | [chapter-31-road-ahead](../chapter-31-road-ahead/) |
| 附录/环境 | [appendix-A-dev-env](../appendix-A-dev-env/) |
| 全书导读 | [../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md) |

---

← [5. 关窗](./section-5-窗口关闭按钮.md) · [Ch 29](../chapter-29-ipc/) · [Ch 30 导读](../README.md)
