# TLPI 第 35 章 — Process Priorities and Scheduling

**优先级**：🔴（嵌入式 / HFT 调度与亲和）  
**前置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/notes.md)  
**后置**：[Ch36 进程资源](../chapter-36-process-resources/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 小节目录

- [35.1 –35.2 Nice（`SCHED_OTHER`）](./notes/35.1-schedother.md)
- [35.3 三大策略](./notes/35.3-strategy.md)
- [35.4 调度 API](./notes/35.4-api.md)
- [35.5 CPU 亲和（Linux）](./notes/35.5-cpu.md)
- [35.6 权限与限制](./notes/35.6-permission-limits.md)
- [35.7 fork / exec](./notes/35.7-fork-exec.md)
- [35.8 实践陷阱（HFT / 嵌入式）](./notes/35.8-hft.md)

---

## 章节目标


nice；`SCHED_OTHER` vs FIFO/RR；`sched_*` API；权限与 `RLIMIT_RTPRIO`；CPU 亲和；`SCHED_RESET_ON_FORK`；实时风险。

---


---

## 实验清单


1. nice get/set  
2. （需 root）FIFO 抢占  
3. RR interval  
4. affinity  
5. （选）`SCHED_RESET_ON_FORK`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | nice：-20…19，只管 OTHER |
| 2 | FIFO/RR：1–99，压过 OTHER |
| 3 | FIFO 无同级时间片；RR 有 |
| 4 | 实时需 CAP_SYS_NICE / RTPRIO |
| 5 | 亲和绑核减抖动 |
| 6 | RESET_ON_FORK 防子进程继承实时 |

---


---

## 参考


- Kerrisk · TLPI Ch35  
- `man 2 setpriority` · `man 2 sched_setscheduler` · `man 2 sched_setaffinity` · `man 7 sched`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
