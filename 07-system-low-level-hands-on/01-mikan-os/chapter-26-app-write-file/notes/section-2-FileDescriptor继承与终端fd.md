## 2. FileDescriptor 继承与终端 fd

---

### 一、为何需要继承

| 后端 | 共性 |
|------|------|
| **FAT 文件** | **Read/Write 字节流** |
| **键盘/终端** | **Read 按键 · Write 显示** |

**Ch25：** `FileDescriptor` 偏 **FAT 读** — 硬编码 **簇/偏移**。

**Ch26：** 抽象 **接口** — 多态 **fd**。

---

### 二、类层次

```cpp
class FileDescriptor {
public:
    virtual size_t Read(void* buf, size_t len) = 0;
    virtual size_t Write(const void* buf, size_t len) = 0;
    virtual ~FileDescriptor() = default;
};

namespace fat {
class FileDescriptor : public ::FileDescriptor {
    DirectoryEntry entry_;
    size_t rd_off_, wr_off_;
    ClusterId cluster_;
    // Read: Ch25 簇链
    // Write: Ch26 AllocateClusterChain
};
}

class TerminalFileDescriptor : public FileDescriptor {
    Task* terminal_task_;
    // Read: 从终端键队列
    // Write: PutString / Draw
};
```

| 派生类 | 绑定 |
|--------|------|
| **fat::FileDescriptor** | **磁盘条目** |
| **TerminalFileDescriptor** | **启动该应用的 TaskTerminal** |

→ [Ch25 FileDescriptor](../chapter-25-app-read-file/notes/section-4-文件描述符与FileDescriptor.md)

---

### 三、启动应用时预置 fd

```cpp
void CallApp(Task* t) {
    t->files_[0] = MakeTerminalFD(t);  // stdin
    t->files_[1] = MakeTerminalFD(t);  // stdout
    t->files_[2] = MakeTerminalFD(t);  // stderr
    RunUser(entry);
}
```

**Newlib `read(0,…)` / `write(1,…)`** 自动 **路由** — 无需应用感知类型。

→ [08 TLPI 标准 fd](../../../06-The-Linux-Programming-Interface/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. stdin](./section-3-stdin回显与Ctrl+D.md)
