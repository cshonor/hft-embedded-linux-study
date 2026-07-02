## 6. printk 与小结

---

### 一、为何需要 `printk()`

| Console 实例 | **全局 printk** |
|--------------|-----------------|
| 需传递 `Console*` | **任意文件** 直接打日志 |
| 适合单一入口 | 仿 **Linux 内核** 调试习惯 |

**目标：** 中断处理、内存分配失败等 — **一行格式化输出** 到屏幕。

---

### 二、变长参数实现

```cpp
void printk(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);   // Newlib
    va_end(ap);
    global_console.PutString(buf);
}
```

| 机制 | 说明 |
|------|------|
| **`...`** | C 变长参数 |
| **`va_list` / `va_start` / `va_end`** | 遍历实参 |
| **`vsprintf`** | 接受 `va_list` 的 sprintf |

**注意：** 内核早期 **非线程安全** — 单核调试期可接受；多任务后需锁（Ch 13+）。

→ 对照 [03-Linux-Kernel-Development 内核 printk 概念](../../../03-Linux-Kernel-Development/) · [ULK 日志](../../../04-Understanding-Linux-Kernel/)

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **WriteAscii** | 位图字体 · 位运算 |
| **模块 + make** | graphics / font 拆分 |
| **objcopy 字体** | 東雲链入内核 |
| **Console** | 换行 · **memcpy 滚动** |
| **Newlib** | `sprintf` / `vsprintf` |
| **`printk`** | 全局调试「嘴」 |

```
像素 (Ch4) → 字符 (Ch5) → 滚动日志 (Console/printk)
                              ↓
                    Ch7 中断 · Ch8 内存 · …
```

---

### 四、后续索引

| Ch5 主题 | 继续读 |
|----------|--------|
| 鼠标 / PCI | [chapter-06-mouse-pci](../chapter-06-mouse-pci/) 🟡 |
| 中断 | [chapter-07-interrupt-fifo](../chapter-07-interrupt-fifo/) 🔴 |
| 文件系统 | [chapter-17-filesystem](../chapter-17-filesystem/) 🟡 |
| ASCII | [appendix-F-ascii-table](../../appendix-F-ascii-table/) |

---

← [5. Console](./section-5-Console与Newlib.md) · [Ch 4](../chapter-04-pixel-make/) · [Ch 5 导读](../README.md)
