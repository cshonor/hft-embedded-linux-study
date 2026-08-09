## 5. 标准输出重定向

---

### 一、默认 fd 布局（Ch26 复习）

| fd | 默认绑定 |
|----|----------|
| **0** | **TerminalFileDescriptor**（键盘） |
| **1** | **TerminalFileDescriptor**（屏幕 stdout） |
| **2** | **TerminalFileDescriptor**（stderr） |

**重定向目标：** 替换 **files_[1]**（或 **2**）— **不改命令本身**。

---

### 二、解析 `>` 符号

**ExecuteLine 扩展：**

```
echo deadbeef > piyo
     ^cmd    ^args   ^redirect target
```

```cpp
void ParseRedirect(Task* t, char* line) {
    char* gt = strchr(line, '>');
    if (!gt) return;
    *gt = '\0';
    char* filename = trim(gt + 1);
    int fd = OpenFile(filename, O_WRONLY | O_CREAT | O_TRUNC);
    t->files_[1] = MakeFatFD(fd);   // stdout → 文件
}
```

| 规则 | 说明 |
|------|------|
| **文件不存在** | **O_CREAT** — Ch26 **CreateEmptyFile** |
| **覆盖写** | **O_TRUNC**（若实现）— 清空再写 |
| **stderr** | **`2>`** 可扩展 — 本书 **错误仍 fd=2 终端** |

→ [Ch26 O_CREAT](../chapter-26-app-write-file/notes/section-4-O_CREAT与FAT写扩展.md) · [Ch16 分词](../chapter-16-commands/notes/section-3-echo与clear命令.md)

---

### 三、应用与内置命令

**CallApp 前 ParseRedirect** — **子进程/files_ 快照** 式绑定：

```
Terminal 任务解析 > 
  → 临时改 files_[1]
  → 运行 echo/cat/app
  → 恢复 files_[1] 为 Terminal
```

**与 Unix shell：** **子进程继承改 fd** — 本书 **单 Task 内模拟**。

---

← [4. 栈扩容](./section-4-栈扩容与FreeType-Bug.md) · 下一节 [6. PrintToFD](./section-6-PrintToFD重构与小结.md)
