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
- **DPDK** — `hugetlbfs` + `mmap` 大页（→ [13-DPDK](../../../13-dpdk/)）
- **注意：** `MAP_POPULATE`（若可用）启动时预 fault

→ [Ch 10 I/O](../../chapter-10-system-io/)

### 常见陷阱

1. **mmap 不立即分配物理页** — 只创建 VA→文件的映射，首次访问时才 page fault 分配物理页（lazy allocation）
2. **MAP_PRIVATE 写触发 COW，不是共享** — 写操作复制新物理页，原文件不变；MAP_SHARED 才真正共享写入
3. **munmap 不保证 flush** — 已写脏页由内核异步写回；需 `msync(MS_SYNC)` 强制同步

### 自测题

<details>
<summary>Q1: MAP_SHARED 和 MAP_PRIVATE 的核心区别？</summary>

MAP_SHARED：多进程映射同一物理页，写操作互相可见且（映射文件时）写回文件。MAP_PRIVATE：写时复制，各进程独立副本，互不影响，原文件不变。
</details>

<details>
<summary>Q2: fork 后子进程的 mmap 区域如何继承？</summary>

fork 复制页表。MAP_SHARED 区域父子共享同一物理页（写入互相可见）；MAP_PRIVATE 区域标记只读，写时 COW 分配新页。
</details>

<details>
<summary>Q3: HFT 用 mmap 做行情文件 replay 有什么好处？需要注意什么？</summary>

好处：1) 避免 read() 系统调用开销；2) 内核页缓存自动管理；3) 顺序访问局部性好。注意：首次访问 page fault 有延迟，应用 MAP_POPULATE 预 fault 或手动预读。
</details>

<details>
<summary>Q4: DPDK 为什么用 hugetlbfs + mmap 大页？</summary>

大页（2MB/1GB）减少 TLB 项数，避免页表 walk 延迟；hugetlbfs 预留大页池，保证分配不失败；mmap 直接映射到用户空间，零拷贝访问网卡 DMA 缓冲。
</details>

---

← [本章导读](../README.md)
