## 3. stdin、回显与 Ctrl+D

---

### 一、键盘回显（Echo）

**TerminalFileDescriptor::Read** 从 **终端键队列** 取字符：

```
用户按键 → 驱动 → Terminal 任务 → 队列
Read()  dequeue → 若 echo: Write() 同字符到 stdout (fd=1)
```

| Echo | 效果 |
|------|------|
| **开** | 键入 **立即可见** — 与 Ch16 **linebuf** 协同 |
| **关** | 密码输入场景（本书可选） |

→ [Ch16 终端输入](../chapter-16-commands/notes/section-2-终端按键输入与行缓冲.md)

---

### 二、特殊路径 `@stdin`

**OpenFile 识别虚拟路径：**

```cpp
if (path == "@stdin") {
    return STDIN_FILENO;   // 0 — 已绑 TerminalFD
}
```

**用途：**

```
> grep pattern @stdin
(用户键入多行 · Ctrl+D 结束)
```

**grep/readfile** 用 **fopen("@stdin")** 把 **键盘当文件** — **管道前奏**。

---

### 三、Ctrl+D — EOT / EOF

**键盘无物理 EOF** — **Read 会无限等**。

**约定：** **Ctrl+D** → ASCII **0x04 (EOT)**

```cpp
size_t TerminalFileDescriptor::Read(void* buf, size_t len) {
    char c = WaitKey();
    if (c == 0x04) return 0;   // read() 返回 0 = EOF
    …
}
```

| 语义 | POSIX 对齐 |
|------|------------|
| **read 返回 0** | **EOF** — `fgets` / `grep` **循环退出** |
| **非关闭 fd** | 可 **再次 read**（若实现支持）— 本书 **一次会话** |

**解决：** `grep foo @stdin` **不再死循环**。

---

← [2. fd 继承](./section-2-FileDescriptor继承与终端fd.md) · 下一节 [4. O_CREAT](./section-4-O_CREAT与FAT写扩展.md)
