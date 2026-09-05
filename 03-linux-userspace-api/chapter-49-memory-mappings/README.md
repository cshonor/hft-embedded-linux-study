# TLPI 第 49 章 — Memory Mappings

**优先级**：🔴（文件 IO / 分配 / IPC 交汇）  
**前置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/README.md)  
**后置**：[Ch50 虚拟内存操作](../chapter-50-virtual-memory/README.md) · [Ch51 POSIX IPC](../chapter-51-posix-ipc-intro/README.md)

> 源码核验基准：Linux v6.6 · `mm/mmap.c` · `mm/msync.c` · `mm/mremap.c`（2026-09-05 实测）

---

## 小节目录

- [49.1 四大组合](notes/49.1-overview.md)
- [49.2 创建映射 `mmap`](notes/49.2-creating-a-mapping-mmap.md)
- [49.3 解除映射 `munmap`](notes/49.3-unmapping-a-mapped-region-munmap.md)
- [49.4 文件映射](notes/49.4-file-mappings.md)
- [49.5 同步映射区 `msync`](notes/49.5-synchronizing-a-mapped-region-msync.md)
- [49.6 flags 参数补充细节](notes/49.6-additional-mmap-flags.md)
- [49.7 匿名映射](notes/49.7-anonymous-mappings.md)
- [49.8 重新映射 `mremap`](notes/49.8-remapping-a-mapped-region-mremap.md)
- [49.9 MAP_NORESERVE 与 overcommit](notes/49.9-map-noreserve-and-swap-space-overcommitt.md)
- [49.10 MAP_FIXED 标志](notes/49.10-the-map-fixed-flag.md)
- [49.11 非线性映射 remap_file_pages](notes/49.11-nonlinear-mappings-remap-file-pages.md)
- [49.12 总结](notes/49.12-summary.md)
- [49.13 练习](notes/49.13-exercises.md)

---

## 章节目标

`mmap`/`munmap`/`msync`；四大组合；PRIVATE vs SHARED；SIGBUS；匿名映射；与 read/SysV shm 对比；v6.6 内核路径全程核验。

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

---

## 易错清单

1. `mmap` 失败返回 **MAP_FAILED**（(void*)-1），不是 NULL
2. `offset` 必须**页对齐**（EINVAL）；length 内核向上取整
3. **PRIVATE 未写过的页跟随 page cache**——不是快照（实测三连，copy-on-write 非 copy-on-access）
4. 写页内 1 字节 = **整页** COW 定格（粒度是页）
5. 越过文件尾访问映射 → **SIGBUS**（不是 SIGSEGV——VMA 合法但无后备）
6. `msync` 只对 SHARED 文件映射有意义；**MS_ASYNC 是 no-op**（2.6.19+）
7. `MS_INVALIDATE` 遇 mlock 区 → **EBUSY**
8. munmap 的 addr 必须**页对齐**（EINVAL）；区间内空洞静默忽略
9. munmap 中间一页 → VMA 分裂（maps 1→2）；相邻同 flags 分配又会 merge
10. `mremap` 无 MAYMOVE 被挡 → **ENOMEM**；搬家是**页表级零拷贝**
11. `MREMAP_DONTUNMAP` 旧地址读回**零**（不是旧数据）；仅匿名映射
12. `MAP_FIXED` 静默覆盖已有映射（先 munmap 再建）；**NOREPLACE 被占→EEXIST**
13. `remap_file_pages` v6.6 纯 emulation（dmesg 警告 + mmap 绕路）
14. overcommit=2 严格模式下 `MAP_NORESERVE` 被忽略（照样记账）

---

## 实验清单（全部实测：WSL Ubuntu，gcc -O2 -Wall -Wextra 零告警，2026-09-05）

| # | 实验 | 验证点 | 位置 |
|---|------|--------|------|
| 1 | mmap 生命周期三态 | 未触碰 Rss=0 / 读触碰（零页）Rss=0 / 写触碰 Rss=4 | 49.2 |
| 2 | VMA 分裂 | munmap 中间页 maps 1→2；PROT_NONE guard 防吞并 | 49.3 |
| 3 | 可见性三连 | priv[0]='S' 跟随 / priv[4096]='K' 快照 / SIGBUS | 49.4 |
| 4 | msync 语义 | SYNC+ASYNC→EINVAL；mlock+INVALIDATE→EBUSY | 49.5 |
| 5 | MAP_POPULATE | Rss 0 vs 16384 kB；首触 15ms vs 1ms | 49.6 / 49.13 实验 7 |
| 6 | mremap 全景 | 原地/ENOMEM/MAYMOVE/收缩/DONTUNMAP 旧址=0x00 | 49.8 / 49.13 实验 6 |
| 7 | MAP_FIXED 三态 | NOREPLACE→EEXIST；hint 自动挪；裸 FIXED 覆盖 | 49.10 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | mmap 三步：选址（get_unmapped_area）→ 建 VMA（mmap_region）→ 挂树（vma_link） |
| 2 | 画区不填页：mmap 后 Rss=0；首触缺页才分配 |
| 3 | MAP_FAILED ≠ NULL |
| 4 | PRIVATE=copy-on-**WRITE**：未写过的页跟随 page cache |
| 5 | COW 粒度=页：写 1 字节定格整页 |
| 6 | SIGSEGV=无 VMA；SIGBUS=有 VMA 无后备（越 EOF） |
| 7 | close(fd) 后映射存活（vm_file 持引用） |
| 8 | MS_SYNC=真落盘；MS_ASYNC=no-op；PRIVATE 调 msync 无操作 |
| 9 | mremap 搬家=PTE 搬迁零拷贝；无 MAYMOVE 受阻=ENOMEM |
| 10 | MREMAP_DONTUNMAP：新址得数据，旧址读零 |
| 11 | MAP_FIXED 静默覆盖；NOREPLACE=EEXIST |
| 12 | VMA 属性相同且紧贴 → vma_merge 合并（guard 技术的反面） |
| 13 | remap_file_pages 已死（v6.6 emulation） |
| 14 | fork 继承全部映射（语义原样）；exec 全销毁 |

---

## 参考

- Kerrisk · TLPI Ch49
- `man 2 mmap` · `munmap` · `msync` · `mremap` · `remap_file_pages`
- 内核深挖：[06-linux-mm](../../06-linux-mm/README.md)
