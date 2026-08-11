# TLPI 第 49 章 — Memory Mappings

**优先级**：🔴（文件 IO / 分配 / IPC 交汇）  
**前置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/notes.md)  
**后置**：[Ch50 虚拟内存操作](../chapter-50-virtual-memory/notes.md) · [Ch51 POSIX IPC](../chapter-51-posix-ipc-intro/notes.md)

---

## 小节目录

- [49.1 四大组合](./notes/49.1-group.md)
- [49.2 –49.3 `mmap` / `munmap`](./notes/49.2-mmap-munmap.md)
- [49.4 文件映射](./notes/49.4-map.md)
- [49.5 `msync`](./notes/49.5-msync.md)
- [49.6 –49.8 Flags · `mremap`](./notes/49.6-mremap.md)

---

## 章节目标


`mmap`/`munmap`/`msync`；四大组合；PRIVATE vs SHARED；SIGBUS；匿名映射；与 read/SysV shm 对比。

---


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


---

## 参考


- Kerrisk · TLPI Ch49（非「第 15 章」误标）  
- `man 2 mmap` · `munmap` · `msync` · `mremap`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
