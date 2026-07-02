## 3. echo 与 clear 命令

---

### 一、命令解析（极简分词）

**Enter 后** 对 `linebuf_` 处理：

```
> echo hello world
  ^cmd  ^args…
```

| 步骤 | 说明 |
|------|------|
| **找第一个空格** | 前段 = **命令名** · 后段 = **参数** |
| **strcmp / 字符串比较** | 分支到 handler |

**无完整 shell 管道** — 单命令 · 单参数串（本章）。

---

### 二、echo

```cpp
if (strcmp(cmd, "echo") == 0) {
    Print(args);   // 参数原样输出 + 换行
}
```

| 用途 | 验证 **分词** · **参数传递** |
|------|------------------------------|
| 示例 | `> echo hello` → `hello` |

---

### 三、clear

```cpp
if (strcmp(cmd, "clear") == 0) {
    FillClientBlack();
    ResetCursorTopLeft();
}
```

| 效果 | 全终端客户区 **黑色** · 光标 **(0,0)** |
|------|----------------------------------------|
| 价值 | 几行代码 · **实用性强** |

**重绘：** `SendMessage(main, kLayer, Redraw…)` — 经 **Main 合成**。

---

← [2. 行缓冲](./section-2-终端按键输入与行缓冲.md) · 下一节 [4. lspci](./section-4-lspci命令.md)
