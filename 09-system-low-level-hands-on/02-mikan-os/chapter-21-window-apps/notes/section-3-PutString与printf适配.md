## 3. PutString 与 printf 适配

---

### 一、PutString 系统调用

**演进 Ch20 `0x80000000`：** 更完整的 **终端字符串输出** — **按调用者任务路由**。

```cpp
int64_t SyscallPutString(const char* s) {
    Task* caller = GetCurrentTask();      // syscall 入口已关联
    Terminal* term = FindTerminalByTaskId(caller->id());
    term->Print(s);
    return 0;
}
```

| 设计 | 说明 |
|------|------|
| **每应用独立 Terminal 任务** | Ch15 **TaskTerminal** 启动应用 |
| **任务 ID 映射** | 输出到 **正确终端窗** — 非全局单一 console |

→ [Ch15 TaskTerminal](../chapter-15-terminal/) · [Ch20 PutString 雏形](../chapter-20-syscall/notes/section-6-终端打印syscall与小结.md)

---

### 二、Newlib 与 write()

**C 库路径：**

```
printf → vfprintf → write(fd, buf, len) → SyscallPutString
```

```c
// apps/newlib stub
ssize_t write(int fd, const void* buf, size_t len) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        SyscallInvoke(SYS_PUT_STRING, buf, len);
        return len;
    }
    return -1;
}
```

| 层次 | 谁实现 |
|------|--------|
| **应用** | `printf` — **标准格式化** |
| **stub** | **`write`** — 薄封装 |
| **内核** | **PutString** — 真正 **Draw/Print** |

→ [Ch5 Console 与 Newlib](../chapter-05-console-text/notes/section-5-Console与Newlib.md)

---

### 三、用户指针校验

**Ring3 传入 `buf`：** 内核 **逐页探测 User 映射** — 防 **向内核地址写假指针** 读机密。

---

← [2. IST](./section-2-IST与定时器中断栈修复.md) · 下一节 [4. exit](./section-4-exit系统调用与CallApp栈恢复.md)
