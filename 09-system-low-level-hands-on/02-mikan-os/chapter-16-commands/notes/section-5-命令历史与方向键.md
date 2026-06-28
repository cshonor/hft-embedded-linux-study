## 5. 命令历史与方向键

---

### 一、功能

**↑ / ↓** 在 **历史命令** 间切换 — 现代终端标配。

| 键 | 行为 |
|----|------|
| **↑** | 上一条（更旧） |
| **↓** | 下一条（更新） |
| 选中历史 | **替换当前 linebuf_** · 重绘行 |

---

### 二、std::deque 环形 8 条

```cpp
std::deque<std::string> history_;  // max 8
int history_index_;                // -1 = 新空行
```

| 规则 | 说明 |
|------|------|
| **Enter 提交** | push_back · 超 8 则 pop_front |
| **浏览** | index 增减 · 取 `history_[index]` |

→ [Ch11 std::deque 引入](../chapter-11-timer-acpi/notes/section-2-源码重构.md)

---

### 三、陷阱：int -1 与 size_t 比较

```cpp
// BUG 示例：
size_t i = history_index_;  // history_index_ == -1
if (i < history_.size()) { … }  // -1 → 巨大无符号 → 真！
```

| 原因 | **有符号 -1** 赋给 **size_t** → **SIZE_MAX** |
|------|-----------------------------------------------|
| 修复 | **保持 int** 比较 · 或 **显式 < 0 判断** · **`std::optional<size_t>`** |

**书中强调：** 嵌入式/内核 C++ **类型混用** 常见 bug — 与 [CSAPP 隐式转换](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/) 呼应。

---

← [4. lspci](./section-4-lspci命令.md) · 下一节 [6. TaskB](./section-6-删除TaskB与小结.md)
