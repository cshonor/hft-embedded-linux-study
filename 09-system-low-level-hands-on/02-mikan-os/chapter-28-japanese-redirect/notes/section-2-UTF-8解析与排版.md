## 2. UTF-8 解析与排版

---

### 一、为何 UTF-8

| 需求 | 选择 |
|------|------|
| **日文 + 英文混排** | **UTF-8** — 事实标准 · **变长编码** |
| 硬编码位图 | 假名/汉字 **量级太大** — 不可行 |

**与终端/GUI 共用：** **WinWriteString · Terminal Print** 均走 **UTF-8 字节流**。

---

### 二、核心函数

```cpp
size_t CountUTF8Size(const char* s);           // 字节长度 → 字符数（码位计数）
char32_t ConvertUTF8To32(const char*& p);      // 取下一 Unicode 码位，p 前进

bool IsHankaku(char32_t cp);                   // 半角（等宽 1 列）
// 全角（日文等）→ 终端占 2 列宽度
```

| UTF-8 结构（复习） | 说明 |
|--------------------|------|
| `0xxxxxxx` | ASCII 1 字节 |
| `110xxxxx 10xxxxxx` | 2 字节 |
| `1110xxxx …` | 3 字节（多数日文） |
| `11110xxx …` | 4 字节 |

**错误处理：** 非法序列 — **替换字符 U+FFFD** 或 **跳过**（按书实现）。

---

### 三、光标与换行

**Ch16 按 **单字节** 前进光标 — 日文需 **显示宽度**：

```
"あA" → 全角2 + 半角1 = 视觉 3 列
Scroll1 / 行尾 wrap 按 **列宽** 非 **字节数**
```

| IsHankaku | 宽度 |
|-----------|------|
| **true** | 1 |
| **false** | 2（CJK 全角） |

→ [Ch16 终端行缓冲](../chapter-16-commands/notes/section-2-终端按键输入与行缓冲.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. FreeType](./section-3-FreeType与矢量字体.md)
