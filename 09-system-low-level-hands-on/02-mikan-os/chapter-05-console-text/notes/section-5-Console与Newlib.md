## 5. Console 类与 Newlib

---

### 一、Console 职责

**纯输出控制台** — 管理 **满屏文本** 的显示与 **滚动**。

| 状态 | 说明 |
|------|------|
| **字符缓冲区 / 行缓冲** | 跟踪当前屏幕内容（或逻辑行） |
| **光标 (cursor_x, cursor_y)** | 下一字符位置 |
| **行高 / 列宽** | 由分辨率 ÷ 字符尺寸得出 |

**输出流程：**

```
PutString(s) → 逐字符
    普通字符 → 当前光标 WriteAscii，cursor_x++
    '\n'     → 换行
    到达底边 → Scroll()
```

---

### 二、滚动机制

屏幕已满时再输出 → **整体上移一行**：

```cpp
void Console::Scroll() {
    // 帧缓冲中：将第 2 行~末行 复制到 第 1 行~
    memcpy(fb + 0, fb + line_bytes, (height - 1) * line_bytes);
    // 清空最后一行
    memset(fb + (height - 1) * line_bytes, 0, line_bytes);
    cursor_y = height - 1;  // 或等价逻辑
}
```

| 要点 | 说明 |
|------|------|
| **`memcpy`** | 按 **字节行** 块移动 — 依赖固定 **pitch** |
| **性能** | 全屏滚动 O(像素) — 调试够用；GUI 终端后续可优化 |
| **`\n`** | 触发换行；底行再触发 Scroll |

→ [CSAPP Ch3 memcpy 语义](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/)

---

### 三、Newlib 与 `sprintf()`

要输出 **`value = 42`** 这类 **格式化字符串** — 自写整数转串繁琐。

**Newlib** — 面向 **嵌入式 / bare metal** 的 C 标准库实现（可裁剪）。

| 函数 | 用途 |
|------|------|
| **`sprintf(buf, "x=%d", x)`** | 格式化写入缓冲 |
| **`snprintf`** | 带长度限制 — 更安全 |

**集成要点（概念）：**

- 链接 **newlib** 或/newlib-nano 子集
- 实现 **`_write` / syscalls stub** — 许多/newlib 函数依赖底层 I/O；内核阶段常 **仅先用 sprintf 族**（纯缓冲）

**Console 用法：**

```cpp
char line[128];
sprintf(line, "memmap entries: %lu", count);
console.PutString(line);
```

---

← [4. 字体嵌入](./section-4-外部字体嵌入.md) · 下一节 [6. printk](./section-6-printk与小结.md)
