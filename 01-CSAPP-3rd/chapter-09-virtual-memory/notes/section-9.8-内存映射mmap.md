## 9.8 内存映射 mmap

### 9.8.1 共享对象

- **`MAP_SHARED`** — 多进程映射同一文件/同一物理页，**写可见**
- **`MAP_PRIVATE`** — 写时复制 (COW)，互不影响

### 9.8.2–9.8.3 再看 fork 与 execve

- **`fork`** — 复制页表，共享 **只读** 页；写时复制私有页
- **`execve`** — 丢弃旧地址空间，映射 ELF **LOAD** 段

### 9.8.4 用户级 mmap

```c
void *mmap(void *addr, size_t len, int prot, int flags,
           int fd, off_t offset);
int munmap(void *addr, size_t len);
```

| 用途 | flags |
|------|-------|
| 读文件 | `MAP_PRIVATE`, `PROT_READ` |
| 共享 IPC | `MAP_SHARED` + `shm_open` / 文件 |
| 匿名堆外缓冲 | `MAP_ANONYMOUS \| MAP_PRIVATE` |
| 锁内存 | `MAP_LOCKED` / 后接 `mlock` |

```c
char *p = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
mlock(p, size);  // 避免换出
```

**HFT：**

- **大文件 replay** — `mmap` 行情文件，顺序读、内核页缓存
- **环形缓冲 / 大数组** — 匿名 `mmap` 替代大 `malloc`
- **DPDK** — `hugetlbfs` + `mmap` 大页（→ [12-DPDK](../../../12-DPDK-Low-Latency-Network/)）
- **注意：** `MAP_POPULATE`（若可用）启动时预 fault

→ [Ch 10 I/O](../chapter-10-system-io/)

---

← [本章导读](../README.md)
