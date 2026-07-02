## 4. 文件描述符与 FileDescriptor

---

### 一、fd 抽象

**效仿 POSIX：** 应用只持 **整数 fd** — 内核隐藏 **FAT 簇/偏移**。

```
open("readme.txt") → fd = 3
read(fd, buf, 512)
close(fd)
```

| 好处 | 说明 |
|------|------|
| **换 FS 实现** | 应用 **不变**（理想） |
| **多文件** | **fd 0,1,2** stdin/stdout/stderr · **≥3** 普通文件 |

→ [08 TLPI 文件 I/O](../../../06-The-Linux-Programming-Interface/)

---

### 二、Task.files_

**Ch24 每任务独立地址空间 — 打开文件亦 **per-Task**：**

```cpp
class Task {
    std::vector<std::unique_ptr<FileDescriptor>> files_;
public:
    int AllocateFD(std::unique_ptr<FileDescriptor> fd);
    FileDescriptor* GetFD(int fd);
};
```

| 规则 | 说明 |
|------|------|
| **fd 索引** | 通常 **0,1,2 预置** · **3+** 动态分配 |
| **进程退出/KillApp** | **关闭并清空 files_** |

→ [Ch24 Task](../chapter-24-multi-terminal/notes/section-3-每应用PML4与CR3切换.md)

---

### 三、FileDescriptor 类

```cpp
class FileDescriptor {
    DirectoryEntry entry_;
    size_t rd_off_;        // 字节读取偏移
    ClusterId rd_cluster_; // 当前读簇
    size_t rd_cluster_off_; // 簇内偏移

    size_t Read(void* buf, size_t len);
};
```

**跨簇读取：**

```
while (len > 0) {
    copy from current cluster [rd_cluster_off_ …]
    if need next cluster:
        rd_cluster_ = FAT[rd_cluster_];
        rd_cluster_off_ = 0;
    rd_off_ += n;
}
```

**与 Ch18 cat 簇链逻辑复用** — 封装进 **面向 fd 的对象**。

---

← [3. apps/](./section-3-apps目录与APPS_DIR.md) · 下一节 [5. syscall/Newlib](./section-5-OpenFile-ReadFile与Newlib.md)
