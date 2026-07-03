## 4. GOP 与帧缓冲区

> 图形界面不能只靠 **文本 ConOut** — 需要 **像素级** 访问屏幕。

---

### 先纠正直觉：GOP 传的不是「屏幕画面」

**不完全是「把屏幕画面传给内核」。** 准确分两层：

#### ① Loader 交给内核的是什么？

**GOP 传给内核的不是当前屏幕像素，而是「显存硬件的地址与配置信息」。**

| GOP 读出 | 含义 |
|----------|------|
| **`FrameBufferBase`** | 帧缓冲 **物理内存起始地址** — 写这块内存，像素就变 |
| **分辨率** | 水平 × 垂直像素数 |
| **`PixelsPerScanLine`** | 每行像素长度（stride，可能大于宽度） |
| **像素格式** | 如 BGR Reserved 8-bit |

MikanLoader 把这堆参数打包（如 `FrameBufferConfig`）传给 **`KernelMain`**。  
内核拿到地址后 **自己读写这块内存** 就能控制显示 —— **不是** 固件把现成画面发过去。

#### ② 开机那一刻屏幕上是什么？

UEFI / Loader 刷白、Logo、文字 —— 只是 **临时画在显存上的图案**。  
内核拿到 `FrameBufferBase` 后，可以 **直接覆盖、清屏、重画** 文字/图形。

**通俗类比：** GOP 相当于告诉你 —— *「画布在内存 **0xXXXX**，尺寸 **1920×1080**，颜料格式 **BGR**」*。  
它 **不会** 把已经画好的图交给内核，只把画布的 **位置与规格** 交给内核；**内核自己在画布上作画**。

→ Linux 同源：[§四 `/dev/fb0`](#四与-linux-devfb0-同源) · 传参详 [§5 KernelMain](./section-5-KernelMain与错误处理.md)

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

Loader 不应独占显示 — **ExitBootServices 前** 启动内核时传递 **硬件参数，不传像素快照**：

| 参数（示意） | 含义 |
|--------------|------|
| **`FrameBufferConfig`** / **fb_base** | 帧缓冲 **基址**（画布在哪） |
| **width / height / stride** | 画布 **尺寸与行宽** |
| **pixel_format** | **BGR / RGB** 等字节序 |

```cpp
extern "C" void KernelMain(const FrameBufferConfig* config, /* … */) {
    // 内核按 config->frame_buffer 基址自己写像素 — 不依赖 UEFI ConOut
}
```

**设计原则：**

```
Loader：GOP 探测 → 打包地址/格式/分辨率 → 交给 KernelMain
Kernel：持久拥有「画布规格」→ 自己清屏、重绘 → Ch10+ 窗口系统的基础
```

→ 下一章 [Ch4 像素与 make](../chapter-04-pixel-make/) 继续细化绘图

---

### 四、与 Linux `/dev/fb0` 同源

| | **MikanOS（GOP → KernelMain）** | **嵌入式 Linux** |
|---|--------------------------------|------------------|
| **拿到什么** | `FrameBufferBase` + 分辨率 + 像素格式 | `/dev/fb0` 映射的 **显存物理地址** |
| **怎么显示** | 内核 **读写该物理内存** | 内核 / 用户态 **mmap 帧缓冲** 写像素 |
| **本质** | 同一套逻辑 — **显存 = 一块可写内存** | 同上 |

**HFT / 嵌入式读法：** 无桌面环境的板卡、KVM 帧缓冲调试 — 都是 **地址 + 格式**，不是「传一张截图」。

---

### 五、与文本输出的关系

| 方式 | 阶段 | 特点 |
|------|------|------|
| **ConOut->OutputString** | Ch 1–2 调试 | 简单文本 · UEFI 服务 |
| **GOP 写像素** | Ch 3+ | **图形 UI** · 需自管字体/位图（Ch 5） |

### 口述巩固

1. **GOP 传给内核的是屏幕截图吗？** — **不是**；是 **FrameBufferBase + 分辨率 + 格式**。
2. **内核怎么显示？** — 按基址 **自己读写显存**；可覆盖 UEFI 临时画面。
3. **和 Linux 哪一样？** — **`/dev/fb0` 帧缓冲** — 同一套「显存地址 + 写内存出图」逻辑。

---

← [3. 内核与 ELF](./section-3-第一个内核与ELF加载.md) · 下一节 [5. KernelMain](./section-5-KernelMain与错误处理.md)
