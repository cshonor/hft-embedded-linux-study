# TLPI 第 50 章 — Virtual Memory Operations

> 对应目录：`chapter-50-virtual-memory/`  
> （勿用 `…-advanced-memory-mapping` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 一致）  
> 书名原文：**Virtual Memory Operations**  
> ⚠️ **`addr` 须页对齐。** `mlock` 受 `ulimit -l` 限制。`mincore` 只是瞬时快照。Linux 上 `MADV_DONTNEED` 对 **PRIVATE** 会丢修改（移植坑）。JIT：先写再改 `RX`（注意 W^X）。

**优先级**：🔴（低延迟 / JIT / 大映射调优）  
**前置**：[Ch49 mmap](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/notes.md)

---

## 章节目标

`mprotect` · `mlock*` · `mincore` · `madvise`；页对齐约束；实时锁定与 advice 语义。

---

## 50.1 总览

四组调用作用于**已有**映射（mmap / 堆 / 栈 均可）：改保护、锁 RAM、查驻留、给访问提示。  
通用：`addr` 页对齐；`length` 上对齐页。

---

## 50.2 `mprotect`

```c
int mprotect(void *addr, size_t length, int prot);
```

`PROT_NONE|READ|WRITE|EXEC`。违例 → **SIGSEGV**。  
不能超过后备权限（只读文件映射 → 不可 `PROT_WRITE` → `EACCES`）。  
JIT：`W` 写码 → 改成 `R|X`；现代常强制 **W^X**（不可同时 W+X）。

---

## 50.3 `mlock` / `mlockall`

```c
int mlock(const void *addr, size_t length);
int munlock(const void *addr, size_t length);
int mlockall(int flags);   /* MCL_CURRENT | MCL_FUTURE */
int munlockall(void);
```

禁 swap，减缺页 IO 延迟（实时 / HFT）。  
- 用户有 **`ulimit -l`** 上限；超限失败  
- 同区多次 `mlock`，**一次 `munlock` 即解**（非叠加计数）  
- `fork` 继承锁；**`exec` 全解锁**  
- `MCL_FUTURE`：后续分配也锁 → 易耗尽限额  
慎用：挤占 RAM → 他进程 OOM。

---

## 50.4 `mincore`

```c
int mincore(void *addr, size_t length, unsigned char *vec);
```

`vec[i] & 1`：页是否在物理内存。  
⚠️ **瞬时、非原子**；不能当同步条件。

---

## 50.5 `madvise`

```c
int madvise(void *addr, size_t length, int advice);
```

内核**可忽略**。常用：

| advice | 含义 |
|--------|------|
| `MADV_NORMAL` | 默认 |
| `MADV_RANDOM` | 少预读 |
| `MADV_SEQUENTIAL` | 激进预读、用过可早收 |
| `MADV_WILLNEED` | 提前装入 |
| `MADV_DONTNEED` | Linux PRIVATE：**丢本地修改**；SHARED 多为丢页缓存语义 |

Linux 另有 `MADV_FREE` 等。

Demo：[`code/`](./code/)

---

## 对比速记

| 调用 | 作用 | 权限 |
|------|------|------|
| mprotect | 改 r/w/x | 通常不需 root |
| mlock* | 禁 swap | ulimit / 特权 |
| mincore | 驻留快照 | 否 |
| madvise | 访问提示 | 否 |

---

## 陷阱

1. 未页对齐 → `EINVAL`  
2. mprotect 越权 → `EACCES`  
3. mlock 非引用计数叠加  
4. mincore 不可靠同步  
5. `MADV_DONTNEED` + PRIVATE 丢数据（Linux）  
6. `MCL_FUTURE` 耗尽锁定额  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 四件套：protect / lock / mincore / advise |
| 2 | 页对齐；JIT 写完再 RX |
| 3 | mlock 抗 swap；ulimit；exec 解锁 |
| 4 | mincore 仅快照 |
| 5 | madvise 非强制；DONTNEED 移植坑 |
| 6 | 大量 mlock 挤内存 |

---

## 参考

- Kerrisk · TLPI Ch50（非「第 45 章」误标）  
- `man 2 mprotect` · `mlock` · `mincore` · `madvise`
