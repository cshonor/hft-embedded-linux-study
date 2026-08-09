## 4. cat 标准输入与重定向建文件

---

### 一、cat 省略文件名

**升级内置 cat：**

```cpp
if (argc < 2) {
    // 无参数 — 从 stdin 复制到 stdout
    while (ReadDelim(STDIN, '\n', line))
        PrintToFD(STDOUT, line);
} else {
    // 原有：逐文件
}
```

| 场景 | 命令 |
|------|------|
| **管道上游** | `grep x file \| cat` |
| **纯转储** | 配合 **stdin** 读键盘 |

→ [Ch29 cat ReadDelim](../chapter-29-ipc/notes/section-5-sort-cat优化与终端修复.md)

---

### 二、cat > foobar — 快速建文件

**Ch28 重定向 + Ch30 stdin cat：**

```
> cat > foobar
hello
world
^D
> cat foobar
hello
world
```

| 机制 | 说明 |
|------|------|
| **`>`** | **files_[1] → 新 FAT 文件** |
| **cat 无参** | **Read stdin** → **Write stdout(实为文件)** |
| **Ctrl+D** | **EOF** — Ch26 **EOT** |

**调试效率：** 无需 **外部编辑器** — **终端草稿文件**。

→ [Ch28 重定向](../chapter-28-japanese-redirect/notes/section-5-标准输出重定向.md) · [Ch26 stdin](../chapter-26-app-write-file/notes/section-3-stdin回显与Ctrl+D.md)

---

← [3. more](./section-3-more命令与管道按键.md) · 下一节 [5. 关窗](./section-5-窗口关闭按钮.md)
