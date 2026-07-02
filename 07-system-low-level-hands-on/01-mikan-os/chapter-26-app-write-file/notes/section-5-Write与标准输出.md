## 5. Write 与标准输出

---

### 一、fat::FileDescriptor::Write()

```cpp
size_t fat::FileDescriptor::Write(const void* buf, size_t len) {
    while (len > 0) {
        if (wr_off_ at cluster boundary full) {
            if (no next cluster)
                AppendCluster(AllocateClusterChain());
            advance cluster;
        }
        copy to cluster[wr_cluster_off_ …];
        wr_off_ += n;
        update entry.file_size = max(entry.file_size, wr_off_);
    }
    return total_written;
}
```

| 机制 | 说明 |
|------|------|
| **AllocateClusterChain()** | FAT **找空闲簇 · 链到文件末** |
| **更新 directory entry size** | **ls** 可见 **新长度** |
| **跨簇** | 与 Ch25 **Read 对称** |

→ [Ch18 簇链读](../chapter-18-apps/notes/section-2-FAT簇链与cat命令.md)

---

### 二、stdout / stderr — fd 1 / 2

**PutString 等 syscall 重构：**

```cpp
// 旧: 直接 FindTerminalByTask
// 新:
SyscallWrite(fd, buf, len) {
    return CurrentTask()->GetFD(fd)->Write(buf, len);
}
```

| fd | 写入目标 |
|----|----------|
| **1 stdout** | **TerminalFileDescriptor** → 终端屏 |
| **2 stderr** | 同 **终端**（可 **分色** — 可选） |
| **≥3** | **fat::FileDescriptor** 磁盘 |

**printf → write(1,…)** — Ch21 **PutString** 能力 **纳入 fd 体系**。

→ [Ch21 PutString](../chapter-21-window-apps/notes/section-3-PutString与printf适配.md)

---

### 三、Newlib write()

```c
ssize_t write(int fd, const void* buf, size_t count) {
    return SyscallInvoke(SYS_WRITE, fd, buf, count);
}
```

**与 Ch25 read/open 对称** — **stdio 双向打通**。

---

← [4. O_CREAT](./section-4-O_CREAT与FAT写扩展.md) · 下一节 [6. cp](./section-6-cp命令与小结.md)
