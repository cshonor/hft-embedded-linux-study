## 4. O_CREAT 与 FAT 写扩展

---

### 一、OpenFile 与 O_CREAT

```cpp
int SyscallOpenFile(const char* path, int flags) {
    auto entry = fat::FindFile(path);
    if (!entry && (flags & O_CREAT)) {
        entry = fat::CreateEmptyFile(parent_dir, basename);
    }
    if (!entry) return -1;
    return AllocateFD(MakeFatFD(*entry, flags));
}
```

| 标志 | 行为 |
|------|------|
| **O_CREAT** | 不存在 → **新建 0 字节文件** |
| **O_RDONLY / O_WRONLY / O_RDWR** | 控制 **Read/Write** |

---

### 二、CreateEmptyFile

| 步骤 | 说明 |
|------|------|
| 1 | 在目录簇 **找空槽**（0x00 或 0xE5 重用） |
| 2 | 写 **8.3 名 · 属性 · size=0** |
| 3 | **首簇** 可 **延迟分配** 或 **分配空簇** |

**目录簇满：** **ExtendCluster()** — 给 **目录本身** 加 **FAT 链上新簇**。

→ [Ch17 目录项](../chapter-17-filesystem/notes/section-4-目录条目结构.md)

---

### 三、ExtendCluster()

```cpp
ClusterId ExtendCluster(ClusterId last) {
    ClusterId free = FindFreeClusterInFAT();
    FAT[last] = free;
    FAT[free] = EOC;
    ZeroCluster(free);
    return free;
}
```

| 用于 | 场景 |
|------|------|
| **目录扩展** | 条目 **超过每簇容量** |
| **文件 Write** | 数据 **超出当前末簇** — 见下节 **AllocateClusterChain** |

**写 FAT 表：** 更新 **内存卷镜像** — 持久化到 **真实盘** 若后续 **sync**（本书或 **内存 FS**）。

---

← [3. stdin](./section-3-stdin回显与Ctrl+D.md) · 下一节 [5. Write](./section-5-Write与标准输出.md)
