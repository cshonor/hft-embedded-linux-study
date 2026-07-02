## 6. PrintToFD 重构与小结

---

### 一、问题：直接画终端 bypass 重定向

**Ch16 echo/cat：** **Terminal::Print** — **硬编码屏幕** — **`>` 无效**。

---

### 二、PrintToFD()

```cpp
void PrintToFD(int fd, const char* s) {
    Task* t = CurrentTask();
    t->GetFD(fd)->Write(s, strlen(s));
}

void PrintError(const char* s) {
    PrintToFD(STDERR_FILENO, s);   // fd=2 始终终端
}
```

| 函数 | 用途 |
|------|------|
| **PrintToFD(1, …)** | **正常输出** — 可 **重定向到文件** |
| **PrintError** | **错误信息** — **不进 piyo 文件** |

---

### 三、内置命令改造

```cpp
// echo
PrintToFD(STDOUT_FILENO, args);

// cat
while (read ...)
    PrintToFD(STDOUT_FILENO, buf);

// 文件不存在
PrintError("cat: file not found\n");
```

**验证：**

```
> echo deadbeef > piyo
> cat piyo
deadbeef
```

**错误分离：**

```
> cat nosuch > out
cat: nosuch: not found        ← stderr 屏幕
> cat out
(empty — 错误未污染 out)
```

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **UTF-8 + 宽度** | **日文混排** |
| **FreeType · 32MiB 卷** | **矢量字体** |
| **32KiB 栈** | **库栈溢出修复** |
| **`>` + PrintToFD** | **stdout 重定向** |

```
Ch28 日文 + 重定向
    ↓
Ch29 IPC · 管道
```

---

### 五、后续索引

| Ch28 主题 | 继续读 |
|----------|--------|
| IPC/管道 | [chapter-29-ipc](../chapter-29-ipc/) |
| stdio/fd | [chapter-26-app-write-file](../chapter-26-app-write-file/) |
| 终端 | [chapter-16-commands](../chapter-16-commands/) |

---

← [5. 重定向](./section-5-标准输出重定向.md) · [Ch 27](../chapter-27-app-memory/) · [Ch 28 导读](../README.md)
