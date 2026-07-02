## 3. USB 键盘与键码映射

---

### 一、USB HID 键盘

与 **Ch6 鼠标** 同属 **USB HID 类** — 共享 **xHCI** 枚举与中断路径：

| 对比 | 鼠标 | 键盘 |
|------|------|------|
| HID 报告 | 位移、按键 | **Keycode 数组** |
| 驱动层 | 类驱动解析 | **keycode → 字符** |

**按键按下/释放** → xHCI 事件 → ISR **Push Message**（含 keycode）— 同 Ch7 架构。

→ [Ch6 USB 分层](../chapter-06-mouse-pci/notes/section-3-USB分层与xHCI.md)

---

### 二、键码（Keycode）

HID **Usage ID** / 引导协议 **键码** — 非 ASCII，是 **物理键编号**：

```
Keycode 0x04 → 物理键 'a'（无 Shift）
Keycode 0x1E → '1'
...
```

---

### 三、keycode_map

**静态映射表** — 键码 → **无 Shift 时的 ASCII**：

```cpp
const char keycode_map[256] = {
    [0x04] = 'a',
    [0x05] = 'b',
    // ...
};
```

| 流程 | 说明 |
|------|------|
| 收到 keycode | 查表得 char |
| 不可打印 | 忽略或特殊处理（Enter、Backspace — §5） |

→ [appendix-F ASCII](../../appendix-F-ascii-table/) · [Ch5 WriteAscii](../chapter-05-console-text/notes/section-2-WriteAscii与位图字体.md)

---

← [2. FADT](./section-2-FADT解析与定时器校准.md) · 下一节 [4. Shift](./section-4-修改键与Shift映射.md)
