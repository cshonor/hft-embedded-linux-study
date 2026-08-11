# TLPI 第 24 章 — Process Creation

**优先级**：🔴（shell、服务、多进程模型地基）  
**前置**：[Ch23 Timers](../chapter-23-timers-sleeping/notes.md) · [Ch20–22 信号](../chapter-20-signals-fundamentals/notes.md)  
**后置**：[Ch25 进程终止](../chapter-25-process-termination/notes.md) · [Ch26 wait](../chapter-26-monitoring-child-processes/notes.md) · [Ch27 exec](../chapter-27-program-execution/notes.md) · [Ch28 fork/exec 深潜](../chapter-28-process-creation-exec-detail/notes.md)

---

## 小节目录

- [24.1 `fork()`](./notes/24.1-fork.md)
- [24.2 Copy-On-Write（COW）](./notes/24.2-copy-on-write.md)
- [24.4 文件描述符](./notes/24.4-file-descriptor.md)
- [24.5 stdio 缓冲陷阱](./notes/24.5-stdio.md)
- [24.6 多线程 + `fork`（重难点）](./notes/24.6-fork.md)
- [24.7 典型范式](./notes/24.7-section-24-7.md)
- [24.8 `vfork`（了解即可）](./notes/24.8-vfork.md)

---

## 章节目标


掌握 `fork`/COW；理清继承与不继承；fd 共享与 stdio 缓冲陷阱；多线程 `fork` 风险与 `fork+exec` 范式；了解为何少用 `vfork`。

---


---

## 24.3 继承清单（速查）


### ✅ 大致继承 / 共享语义

虚拟地址（COW）· fd 表（打开文件引用 +1，共享偏移）· cwd / umask / PGID / SID · 信号处置 · 凭证 · rlimit · 信号**掩码** 等

### ❌ 不继承 / 特殊

| 项 | 行为 |
|----|------|
| PID / PPID | 新值 |
| **pending 信号** | **清空**（高频考点） |
| 其它线程 | **全部消失**（只留调用 `fork` 的线程） |
| 文件锁 | 通常不继承 |
| 部分定时器 / inotify 等 | 实现相关，勿假设完整继承 |

---


---

## 24.9 易错清单


1. pending 清空，掩码继承  
2. COW ≠ 共享可写内存  
3. fork 前 fflush  
4. 多线程 fork → 死锁风险  
5. fd 共享偏移  
6. 子退出需 `wait`（Ch26）  

---


---

## 练习


1. 打印父子 PID / 返回值  
2. 改全局变量，验证互不影响  
3. 复现/修复 stdio 双份输出  
4. （选）多线程 fork 风险  
5. 父子同 fd `write` 交错  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 一调两返：父=子 PID，子=0 |
| 2 | COW：写时才真正分页 |
| 3 | 子 pending 清空；掩码继承 |
| 4 | 多线程 fork 只留一线程 |
| 5 | fork 前 fflush；fd 共享偏移 |
| 6 | 首选 fork+exec；少用 vfork |

---


---

## 参考


- Kerrisk · TLPI Ch24  
- `man 2 fork` · `man 2 vfork` · `man 3 pthread_atfork`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
