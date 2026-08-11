# TLPI 第 28 章 — Process Creation and Program Execution in More Detail

**优先级**：🔴（多线程 fork、信号/fd 生命周期、clone 关系）  
**前置**：[Ch24](../chapter-24-process-creation/notes.md) · [Ch25](../chapter-25-process-termination/notes.md) · [Ch26](../chapter-26-monitoring-child-processes/notes.md) · [Ch27](../chapter-27-program-execution/notes.md)  
**后置**：[Ch29 线程导论](../chapter-29-threads-intro/notes.md)（同步见 [Ch30](../chapter-30-thread-synchronization/notes.md)）· 凭证见 [Ch9](../chapter-09-process-credentials/notes.md)

---

## 小节目录

- [28.1 fork 再深入](./notes/28.1-fork.md)
- [28.2 多线程 + fork](./notes/28.2-fork.md)
- [28.3 `vfork`](./notes/28.3-vfork.md)
- [28.4 `clone`（Linux）](./notes/28.4-clone.md)
- [28.5 exec 资源细则（承 Ch27）](./notes/28.5-exec-ch27.md)
- [28.6 `FD_CLOEXEC` / `O_CLOEXEC`](./notes/28.6-fdcloexec-ocloexec.md)
- [28.7 fork + exec 规范](./notes/28.7-fork-exec.md)
- [28.8 shebang](./notes/28.8-shebang.md)

---

## 章节目标


深挖 COW 与继承清单；多线程 fork / `pthread_atfork`；`vfork`/`clone`；exec 资源细则与 fork/exec 信号对比表；`O_CLOEXEC`；规范 fork+exec。

---


---

## 28.9 易错清单


1. fork：掩码继承、pending 清；exec：handler→DFL、掩码保留  
2. 多线程 fork → 立刻 exec  
3. 勿用 vfork  
4. CLOEXEC 只对 exec  
5. exec 成功不返回；失败 `_exit`  
6. clone 是 fork/线程共同底座  

---


---

## 实验清单


1. COW：改全局变量互不影响（见 Ch24 `fork_basic`）  
2. （选）多线程 fork 风险  
3. CLOEXEC（Ch27）  
4. fork+exec 重定向模板  
5. fork/exec 信号状态对比（本目录 demo）  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | fork 掩码✓ pending✗；exec 掩码✓ handler→DFL |
| 2 | 多线程 fork 只留一线程 → 立刻 exec |
| 3 | vfork 共享地址空间，新代码禁用 |
| 4 | fork/线程 ⊂ clone(flags) |
| 5 | CLOEXEC：fork 不关、exec 关 |
| 6 | 子失败 `_exit`，防 atexit 双跑 |

---


---

## 参考


- Kerrisk · TLPI Ch28  
- `man 2 fork` · `man 2 vfork` · `man 2 clone` · `man 2 execve` · `man 3 pthread_atfork`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
