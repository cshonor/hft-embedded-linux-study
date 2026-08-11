# TLPI 第 55 章 — File Locking

**优先级**：🔴（文件同步；单实例 pid 文件）  
**前置**：[Ch54 POSIX shm](../chapter-54-posix-shared-memory/notes.md) · 文件 I/O  
**后置**：[Ch56 Sockets 导论](../chapter-56-sockets-intro/notes.md)

---

## 小节目录

- [55.1 模型](./notes/55.1-model.md)
- [55.2 `flock`](./notes/55.2-flock.md)
- [55.3 –55.4 `fcntl` 记录锁](./notes/55.3-fcntl.md)
- [55.5 劝告 vs 强制（Linux）](./notes/55.5-section-55-5.md)
- [55.6 –55.9 对比 · 运维 · 场景](./notes/55.6-comparison-ops.md)

---

## 章节目标


`flock` / `fcntl` 记录锁；绑定对象与 close/fork；劝告 vs 强制；pid 文件；死锁与对比。

---


---

## 陷阱


1. fcntl：关「无关」fd 也放全锁  
2. flock：dup 后 close 放锁  
3. flock+fcntl 混用失效  
4. GETLK 非原子获取  
5. stdio：锁内外 `fflush`；关键路径用 `read`/`write`  
6. NFS 锁不可靠  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 默认劝告；协作才有效 |
| 2 | flock=OFD 整文件；fcntl=进程+区 |
| 3 | fcntl：任意 close → 全锁放 |
| 4 | fork 两类都不继承锁 |
| 5 | 勿混用 flock/fcntl |
| 6 | pid 文件用 F_WRLCK 单实例 |

---


---

## 参考


- Kerrisk · TLPI Ch55（非「第 14 章」误标）  
- `man 2 flock` · `fcntl` · `man 3 lockf`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
