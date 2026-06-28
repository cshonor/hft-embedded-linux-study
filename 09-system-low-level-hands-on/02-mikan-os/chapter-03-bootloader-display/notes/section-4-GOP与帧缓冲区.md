## 4. GOP 与帧缓冲区

> 图形界面不能只靠 **文本 ConOut** — 需要 **像素级** 访问屏幕。

---

### 一、Graphics Output Protocol（GOP）

**GOP** — UEFI **图形输出协议**，替代 legacy VGA 文本模式的现代路径。

| 获取信息 | 用途 |
|----------|------|
| **Frame Buffer 首地址** | 像素数组在 **物理内存** 中的起点 |
| **分辨率** | 水平 × 垂直像素数 |
| **像素格式** | 如 **BGR Reserved 8-bit**（每像素 32 bit，蓝绿红 + 保留） |
| **FrameBufferSize** | 缓冲区总字节数 |

**调用路径（概念）：**

```
LocateProtocol(EFI_GRAPHICS_OUTPUT_PROTOCOL)
    → GraphicsOutput->Mode->FrameBufferBase
    → GraphicsOutput->Mode->Info->HorizontalResolution / …
```

---

### 二、在 Loader 中填充白色（实验）

**第一步：** 在 **MikanLoader** 内直接向帧缓冲写像素 — 验证 GOP 可用：

```
对每个像素 offset = (y * width + x) * bytes_per_pixel
framebuffer[offset + 0] = 0xFF;  // B
framebuffer[offset + 1] = 0xFF;  // G
framebuffer[offset + 2] = 0xFF;  // R
→ 全屏白色
```

| 要点 | 说明 |
|------|------|
| **直接写内存** | 帧缓冲 = **MMIO 或 RAM 映射** — 写即显示 |
| **格式必须匹配** | 按 Mode->Info->PixelFormat 解释字节序 |

---

### 三、绘图权移交给内核

Loader 不应独占显示 — **启动内核时传递：**

| 参数（示意） | 含义 |
|--------------|------|
| **fb_base** | 帧缓冲物理/线性基址 |
| **fb_size** 或 **width/height/stride** | 绘图边界 |

```cpp
extern "C" void KernelMain(void* fb_base, uint64_t fb_size) {
    // 内核内绘制彩色图案 — 不再依赖 UEFI ConOut
}
```

**设计原则：**

```
Loader：硬件探测 + 资源分配 + 一次性交接
Kernel：持久拥有帧缓冲 — 后续窗口系统（Ch 10+）的基础
```

→ 下一章 [Ch4 像素与 make](../chapter-04-pixel-make/) 继续细化绘图

---

### 四、与文本输出的关系

| 方式 | 阶段 | 特点 |
|------|------|------|
| **ConOut->OutputString** | Ch 1–2 调试 | 简单文本 · UEFI 服务 |
| **GOP 写像素** | Ch 3+ | **图形 UI** · 需自管字体/位图（Ch 5） |

---

← [3. 内核与 ELF](./section-3-第一个内核与ELF加载.md) · 下一节 [5. KernelMain](./section-5-KernelMain与错误处理.md)
