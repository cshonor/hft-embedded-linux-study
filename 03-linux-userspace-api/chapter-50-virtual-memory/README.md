# TLPI 第 50 章 — Virtual Memory Operations

**优先级**：🔴（低延迟 / JIT / 大映射调优）  
**前置**：[Ch49 mmap](../chapter-49-memory-mappings/README.md)  
**后置**：[Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/README.md)

> 源码核验基准：Linux v6.6 · `mm/mprotect.c` · `mm/mlock.c` · `mm/mincore.c` · `mm/madvise.c`（2026-09-05 实测）

---

## 小节目录

- [50.1 修改保护 `mprotect`](notes/50.1-changing-memory-protection-mprotect.md)
- [50.2 内存锁定 `mlock` / `mlockall`](notes/50.2-memory-locking-mlock-and-mlockall.md)
- [50.3 驻留查询 `mincore`](notes/50.3-determining-memory-residence-mincore.md)
- [50.4 访问提示 `madvise`](notes/50.4-advising-future-memory-usage-patterns-ma.md)
- [50.5 总结](notes/50.5-summary.md)
- [50.6 练习](notes/50.6-exercises.md)

---

## 章节目标

`mprotect` · `mlock*` · `mincore` · `madvise`；页对齐约束；实时锁定与 advice 语义；四个 v6.6 实测锚点。

---

## 对比速记

| 调用 | 作用 | 权限 | 关键实测结论 |
|------|------|------|-------------|
| mprotect | 改 r/w/x | 不超 fd 模式（EACCES） | 违例 SIGSEGV；W^X 加固 |
| mlock* | 禁 swap | RLIMIT_MEMLOCK / CAP_IPC_LOCK | **超限 ENOMEM（非 EPERM）**；MS_INVALIDATE→EBUSY |
| mincore | 驻留快照 | 无权限文件映射静默 0 | 零页 mincore=1 但 Rss=0 |
| madvise | 访问提示 | advice 合法即可 | DONTNEED：匿名→0，文件 PRIVATE→原内容 |

---

## 易错清单

1. `mlock` 超限返回 `ENOMEM`，不是 `EPERM`（EPERM 只在「限额 0 且无 CAP_IPC_LOCK」）
2. `mincore` 对无权限文件映射静默返回全 0（v6.6 `can_do_mincore()` 防侧信道），不是「真的不在内存」
3. 零页：`mincore` 说驻留、`Rss` 不计——两个口径不同
4. `MADV_DONTNEED` 匿名映射读回 **0**；文件 PRIVATE 读回**文件内容**——不是同一件事
5. `MADV_FREE` 是 lazyfree（内存压力前数据还在），不能当确定性清零用
6. `MCL_FUTURE` 让后续 mmap/malloc 静默受限额约束——错误会在离 mlockall 很远的地方爆
7. `mincore` 是瞬时快照，不能做同步条件
8. `mprotect` 的 `addr` 必须页对齐（EINVAL），`length` 内核取整
9. W^X：同页避免 W+X；JIT 写完改 R|X（ARM64 还需 `__clear_cache` 同步 I-cache）

---

## 实验清单（全部实测：WSL Ubuntu，gcc -O2 -Wall -Wextra 零告警，2026-09-05）

| # | 实验 | 验证点 | 位置 |
|---|------|--------|------|
| 1 | mprotect 三段式 | PROT_NONE→SIGSEGV；恢复 RW→写入成功 | 50.1 代码示例 |
| 2 | mlock 限额 | RLIMIT 64MB（WSL）；128MB→ENOMEM；单页成功 | 50.2 代码示例 |
| 3 | mincore 零页口径 | [1,1,0,0] vs Rss=4kB | 50.3 代码示例 |
| 4 | MADV_DONTNEED 双语义 | 匿名归零 0x00；文件 PRIVATE 回读 'O'；EINVAL | 50.4 代码示例 / 50.6 实验 4 |
| 5 | mlock 限额错误码 | ENOMEM 断言 + 超限不耗物理内存 | 50.6 实验 5 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | mprotect 违例 → **SIGSEGV**；超 fd 模式 → **EACCES** |
| 2 | mlock 超限 → **ENOMEM**；无资格（limit=0 无 CAP_IPC_LOCK）→ **EPERM** |
| 3 | mlock 语义：**立即预填充**（`__mm_populate`）+ 无计数（一次 munlock 全解） |
| 4 | fork **继承**锁，exec **解锁** |
| 5 | mlock 区上 `msync(MS_INVALIDATE)` → **EBUSY** |
| 6 | mincore 口径 = **PTE present**；零页 resident 但 Rss 不计 |
| 7 | mincore 无权限文件映射 → **静默全 0**（防侧信道） |
| 8 | THP 在 mincore 里整块 2MB 全 1 |
| 9 | MADV_DONTNEED：匿名→**丢成 0**；文件 PRIVATE→**丢 COW 读回文件** |
| 10 | MADV_FREE：lazyfree，压力前数据保留，可反悔 |
| 11 | mlockall 的 MCL_FUTURE 陷阱：远处 malloc 失败 |
| 12 | 四组调用 addr 均**页对齐**，length 向上取整 |

---

## 参考

- Kerrisk · TLPI Ch50（非「第 45 章」误标）  
- `man 2 mprotect` · `mlock` · `mincore` · `madvise`
- 内核深挖：[06-linux-mm](../../06-linux-mm/README.md)
