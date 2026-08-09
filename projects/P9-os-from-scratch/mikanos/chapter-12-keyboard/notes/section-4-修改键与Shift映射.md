## 4. 修改键与 Shift 映射

---

### 一、USB 键盘 8 字节报告（典型）

| 字节 | 含义 |
|------|------|
| **Byte 0** | **Modifier bitmap** — Ctrl/Shift/Alt/GUI 等 |
| Byte 1 | 保留 |
| **Byte 2–7** | 最多 **6 个同时按下的 keycode** |

**Modifier 位（示意）：**

```
bit1 = Left Shift
bit5 = Right Shift
...
```

---

### 二、Shift 状态与第二张表

```cpp
const char keycode_map_shifted[256] = {
    [0x04] = 'A',   // Shift + a
    [0x1E] = '!',   // Shift + 1
    // ...
};
```

| 逻辑 | 说明 |
|------|------|
| 解析 byte0 | **shift_pressed** = 左/右 Shift 任一位 |
| 查表 | shift ? **keycode_map_shifted** : **keycode_map** |

**效果：** **大写字母** 与 **`!@#$%`** 等符号可输入。

---

### 三、其他修饰键（概念）

| 键 | 本章/后续 |
|----|-----------|
| **Ctrl / Alt** | 可扩展快捷键 — 终端/Ch16 命令 |
| **Caps Lock** | 需 **锁定状态** — 比 momentary Shift 多一层状态机 |

---

← [3. USB 键盘](./section-3-USB键盘与键码映射.md) · 下一节 [5. 文本框](./section-5-GUI文本框与退格.md)
