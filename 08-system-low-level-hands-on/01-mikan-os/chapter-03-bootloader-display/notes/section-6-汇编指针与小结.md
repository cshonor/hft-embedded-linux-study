## 6. 汇编视角下的指针与小结

---

### 一、从汇编理解指针

C/C++ 指针在 x86-64 汇编中体现为 **地址计算 + 内存访问**：

| C 概念 | 汇编（典型） | 含义 |
|--------|--------------|------|
| **`&obj`** | **`lea rax, [obj]`** | **Load Effective Address** — 取地址，不读内存内容 |
| **`*p = v`** | **`mov [rax], edx`** | 把值写入 **rax 指向的内存** |
| **`p->field`** | **`mov eax, [rbx + offset]`** | 基址 + 偏移 |

**`[]` 的含义：** 方括号内是 **有效地址** — CPU 访问该地址处的内存。

```c
uint32_t *fb = (uint32_t*)frame_buffer_base;
fb[x + y * width] = color;
```

可能生成类似：

```asm
; rbx = fb, edi = index, esi = color
mov dword ptr [rbx + rdi*4], esi
```

**帧缓冲绘图 = 指针算术 + 批量 mov** — 无神秘「图形 API」，就是 **写 RAM**。

→ [Ch2 指针基础](../chapter-02-edk2-memmap/notes/section-5-C指针基础.md) · [CSAPP Ch3](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/)

---

### 二、本章总结

| 里程碑 | 说明 |
|--------|------|
| **Loader ∥ Kernel** | MikanLoader (PE) + **kernel.elf (ELF)** |
| **加载链** | 读盘 → 分配 → 解析 **Entry** → 跳转 |
| **GOP** | **FrameBufferConfig**（基址+格式）· Loader 可刷白 · **KernelMain 接管显存** |
| **健壮性** | **EFI_STATUS** + 失败死循环 |
| **调试** | QEMU monitor · **RIP/RFLAGS** |
| **本质** | 指针 = **lea / mov / []** |

```
Ch2 memmap（物理内存账本）
    ↓
Ch3 Loader 加载内核 + GOP + ExitBootServices  ← 分水岭
    ↓
Ch4+ 像素/make · Ch5 文本 · Ch10 窗口…
```

---

### 三、后续索引

| Ch3 主题 | 继续读 |
|----------|--------|
| 像素 / make | [chapter-04-pixel-make](../chapter-04-pixel-make/) ⚪ |
| 控制台文本 | [chapter-05-console-text](../chapter-05-console-text/) ⚪ |
| 内存管理 | [chapter-08-memory](../chapter-08-memory/) 🔴 |
| 窗口 | [chapter-10-window](../chapter-10-window/) ⚪ |
| Ch2 memmap | [chapter-02-edk2-memmap](../chapter-02-edk2-memmap/) |

---

← [5. KernelMain](./section-5-KernelMain与错误处理.md) · [Ch 2](../chapter-02-edk2-memmap/) · [Ch 3 导读](../README.md)
