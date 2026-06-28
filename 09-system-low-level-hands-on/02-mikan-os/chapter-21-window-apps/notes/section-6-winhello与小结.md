## 6. winhello 与小结

---

### 一、winhello 应用

**第一个 GUI 窗口应用：**

```cpp
int main() {
    auto lid = OpenWindow(300, 200, "winhello");
    WinWriteString(lid, 10, 20, 0xFFFFFF, "hello world! line1");
    WinWriteString(lid, 10, 40, 0x00FF00, "hello world! line2");
    WinWriteString(lid, 10, 60, 0xFF0000, "hello world! line3");
    exit(0);
}
```

| 验证点 | 效果 |
|--------|------|
| **OpenWindow** | 弹出 **独立小窗** |
| **WinWriteString ×3** | **不同颜色三行** 文本 |
| **exit(0)** | 关闭应用 · **回到终端/OS** — 非死循环 |

**运行：** 终端 `> winhello`（或磁盘 **ELF 名**）— Ch18 **加载链** + Ch19 **页表** + Ch20 **Ring3**。

---

### 二、本章总结

| 成果 | 说明 |
|------|------|
| **IST1** | **定时器 + syscall** 栈安全 |
| **PutString / printf** | **任务绑定终端** |
| **exit** | **CallApp 内核栈恢复** |
| **syscall.h** | **应用生态公共接口** |
| **OpenWindow / WinWriteString** | **GUI syscall** |
| **winhello** | **端到端演示** |

```
Ch21 应用自建窗口
    ↓
Ch22–24 窗口事件 · 输入 · 重绘
Ch25–28 更多 GUI 应用
```

---

### 三、后续索引

| Ch21 主题 | 继续读 |
|----------|--------|
| 图形事件 | [chapter-22-graphics-events1](../chapter-22-graphics-events1/) ⚪ |
| 图层/窗口内核 | [chapter-09-layers](../chapter-09-layers/) · [chapter-10-window](../chapter-10-window/) |
| syscall 基础 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |

---

← [5. 窗口 syscall](./section-5-syscall.h与窗口系统调用.md) · [Ch 20](../chapter-20-syscall/) · [Ch 21 导读](../README.md)
