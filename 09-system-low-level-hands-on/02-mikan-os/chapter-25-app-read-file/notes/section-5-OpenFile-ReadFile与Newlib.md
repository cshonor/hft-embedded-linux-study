## 5. OpenFile、ReadFile 与 Newlib

---

### 一、内核 syscall

| syscall | 作用 |
|---------|------|
| **OpenFile(path)** | **FindFile** → 新建 **FileDescriptor** → 返回 **fd** |
| **ReadFile(fd, buf, len)** | **files_[fd]->Read** · 校验 **用户 buf** |

```cpp
int64_t SyscallOpenFile(const char* path) {
    auto entry = fat::FindFile(path);
    if (!entry) return -1;
    return CurrentTask()->AllocateFD(MakeFD(*entry));
}

int64_t SyscallReadFile(int fd, void* buf, size_t len) {
    return CurrentTask()->GetFD(fd)->Read(CopyToUser(buf), len);
}
```

→ [Ch20 syscall 表](../chapter-20-syscall/notes/section-5-syscall机制与SyscallEntry.md)

---

### 二、newlib_support.c

**应用链接 Newlib 所需底层钩子：**

```c
int open(const char* path, int flags, …) {
    return SyscallInvoke(SYS_OPEN, path, flags);
}

ssize_t read(int fd, void* buf, size_t count) {
    return SyscallInvoke(SYS_READ, fd, buf, count);
}

void* sbrk(intptr_t increment) {
    return SyscallInvoke(SYS_SBRK, increment);
}
```

| 函数 | 用途 |
|------|------|
| **open/read** | **fopen/fread/fgets** 最终落到此 |
| **sbrk** | **malloc** 堆扩展 — **简化的用户堆**（内核 **brk  syscall** 或 **固定区**） |

→ [Ch5 Newlib](../chapter-05-console-text/notes/section-5-Console与Newlib.md) · [Ch21 write→PutString](../chapter-21-window-apps/notes/section-3-PutString与printf适配.md)

---

### 三、stdio 栈

```
fopen("foo.txt", "r")
  → open()
  → SYS_OPEN → FileDescriptor
fgets(buf, n, fp)
  → read()
  → SYS_READ → 簇链拷贝
```

**应用 **无需** 直接 syscall 号** — **标准 C 即可**。

---

← [4. fd](./section-4-文件描述符与FileDescriptor.md) · 下一节 [6. readfile/grep](./section-6-readfile-grep与小结.md)
