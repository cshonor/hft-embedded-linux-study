# TLPI 第 49 章 — Memory Mappings

> 对应目录：`chapter-49-memory-mappings/`  
> 书名原文：**Memory Mappings**  
> ⚠️ **`MAP_PRIVATE` 写不落盘（COW）；`MAP_SHARED` 才对其他进程/文件可见。** 共享映射越过文件尾 → **SIGBUS**（非 SIGSEGV）。`offset` 须页对齐。后置地图名是 [Ch50 Virtual Memory Operations](../chapter-50-virtual-memory/notes.md)（非 `…-advanced-memory-mapping`）。

**优先级**：🔴（文件 IO / 分配 / IPC 交汇）  
**前置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/notes.md)  
**后置**：[Ch50 虚拟内存操作](../chapter-50-virtual-memory/notes.md) · [Ch51 POSIX IPC](../chapter-51-posix-ipc-intro/notes.md)

---

## 章节目标

`mmap`/`munmap`/`msync`；四大组合；PRIVATE vs SHARED；SIGBUS；匿名映射；与 read/SysV shm 对比。

---

## 49.1 四大组合

| | `MAP_PRIVATE` | `MAP_SHARED` |
|--|---------------|--------------|
| **文件** | COW；改不回文件；加载 text/只读文件 | 改可见且可回盘；无关进程 IPC |
| **匿名** | 私有堆式分配（malloc 一类） | **仅亲缘**（fork 继承）IPC |

生命周期：`fork` 继承映射；**`exec` / 退出** 销毁全部映射。

---

## 49.2–49.3 `mmap` / `munmap`

```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
```

- `addr=NULL` 推荐；`length` 上对齐页  
- `prot`：`NONE/READ/WRITE/EXEC`；**不得超过文件 open 权限**  
- `flags`：必须 **SHARED 或 PRIVATE 二选一**  
- 文件：`fd`+**页对齐** `offset`；匿名：`MAP_ANONYMOUS`，`fd=-1`，`offset=0`  
- 失败：`MAP_FAILED`  
- `munmap` **不**自动刷盘 → 共享映射要落盘用 `msync`

---

## 49.4 文件映射

**PRIVATE**：首写 COW，仅本进程可见，永不写回文件。  
**SHARED**：同页共享；改对其它映射者可见；内核异步回盘。

| 越界 | PRIVATE | SHARED |
|------|---------|--------|
| 映区大于文件 | 零页；写不扩文件 | 越过 EOF → **SIGBUS** |

SIGSEGV = 非法虚址；SIGBUS = 映射合法但无后备。

---

## 49.5 `msync`

```c
int msync(void *addr, size_t length, int flags);
```

`MS_SYNC` 阻塞落盘 · `MS_ASYNC` 后台 · `MS_INVALIDATE` 作废缓存。  
仅对 **SHARED** 有意义。

---

## 49.6–49.8 Flags · `mremap`

| 标志 | |
|------|--|
| `MAP_FIXED` | 强制地址，可盖旧映射 — 危险 |
| `MAP_POPULATE` | 预缺页，减运行时 fault |
| `MAP_NORESERVE` | 不预留 swap；写时可能 SIGSEGV/OOM |

`mremap`：改映射大小（Linux 扩展）。

Demo：[`code/`](./code/)

---

## 横向对比

| | read/write | mmap 文件 |
|--|------------|-----------|
| 拷贝 | 用户↔页缓存两次 | 直接碰页缓存，少一次 |
| 代价 | 系统调用 | 缺页；小顺序 IO 未必更快 |

| | 共享匿名 mmap | SysV shm | 共享文件 mmap |
|--|---------------|----------|---------------|
| 进程 | 仅亲缘 | 任意本机 | 任意本机（同文件） |
| 持久 | 末 munmap 毁 | 内核持久+RMID | 可回盘 |

共享区同样：**勿存裸指针，用 offset**。

---

## 陷阱清单

1. `offset` 未页对齐 → 失败  
2. SHARED 越过 EOF → SIGBUS  
3. PRIVATE 改文件「不生效」  
4. munmap ≠ msync  
5. 匿名 SHARED 不能给无关进程  
6. prot > open 权限  
7. 共享区裸指针  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | PRIVATE=COW 不回盘；SHARED=可见可回盘 |
| 2 | 四组合：私匿/私文/共匿/共文 |
| 3 | fork 继承；exec 全毁 |
| 4 | EOF 越界 SHARED → SIGBUS |
| 5 | 落盘用 msync；匿名 fd=-1 |
| 6 | mmap 大文件随机访问常更省拷贝 |

---

## 参考

- Kerrisk · TLPI Ch49（非「第 15 章」误标）  
- `man 2 mmap` · `munmap` · `msync` · `mremap`
