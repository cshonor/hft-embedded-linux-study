# TLPI 第 35 章 — 进程优先级与调度（Process Priorities and Scheduling）

**优先级**：🔴（HFT/嵌入式调度配置的日常核心；`SCHED_FIFO` + 绑核 + 限流三件事全在本章）
**前置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/README.md)
**后置**：[Ch36 进程资源](../chapter-36-process-resources/README.md) · [Ch37 Daemons](../chapter-37-daemons/README.md)

---

## 小节目录

- [35.1 进程优先级与 nice 值](notes/35.1-process-priorities-nice-values.md)——权重而非优先级；三 API；getpriority 陷阱；TID 粒度；autogroup 修正
- [35.2 实时调度概述](notes/35.2-overview-of-realtime-process-scheduling.md)——两个世界一个 prio 空间；位图 O(1) 选任务；FIFO/RR 行为规则；RT throttling + dmesg 指纹
- [35.3 实时调度 API 详解](notes/35.3-realtime-process-scheduling-api.md)——内核调用链；权限判据源码全解；RESET_ON_FORK 隔代实测；SCHED_IDLE 单向门；yield 三义
- [35.4 CPU 亲和性](notes/35.4-cpu-affinity.md)——绑核语义五细节；迁移五步链路（stopper 线程）；isolcpus 完整隔离栈
- [35.5 总结](notes/35.5-summary.md)——速查表 ×13；反直觉真相 ×4；铁律 ×5；HFT/嵌入式关联表
- [35.6 练习](notes/35.6-exercises.md)——**6 个可实跑实验**（饿死/轮转/限流 + 预写→实测三连：权重比/单向门/隔代）+ 3 道设计题

---

## 章节目标

读完本章应能回答：nice 到底改了内核什么（`sched_prio_to_weight` 查表）；为什么 RT 优先级 1 碾压 nice -20（统一 prio 空间反向映射）；`sched_setscheduler` 每一步校验与权限判据（`user_check_sched_setscheduler`）；FIFO/RR 的精确行为差异（`task_tick_rt`）；满负荷 FIFO 为什么每秒被冻结 50ms（RT throttling 950ms/1s）；绑核与独占核差在哪几层（isolcpus/nohz_full/IRQ）。

---

## 机制全景

```
                    用户视角
   nice -20 … 0 … +19          RT 优先级 1 … 99
        │                            │ NICE_TO_PRIO(+120)        反向映射
        ▼                            ▼ MAX_RT_PRIO-1-rt_prio
   ┌────────────── 内核 p->prio 空间 (越小越优先) ──────────────┐
   │ 0 …… 98 99 │ 100 …… 120 …… 139 │                          │
   │   RT 段    │     CFS(nice) 段   │                          │
   │  FIFO/RR   │  OTHER/BATCH/IDLE  │                          │
   └────────────┴────────────────────┴──────────────────────────┘
        │               │
   rt_sched_class   fair_sched_class     ← 调度类链: stop>dl>rt>fair>idle
        │               │
   rt_prio_array    CFS 红黑树(vruntime)
   位图 O(1) 选任务  权重 = sched_prio_to_weight[nice+20]

   安全网: RT throttling  950ms/1s (sched_rt_runtime_exceeded)
           RLIMIT_RTTIME  每任务 SIGKILL (watchdog)
   物理层: sched_setaffinity 绑核 → isolcpus 隔离 → nohz_full → IRQ 绕行
```

---

## 易错清单

1. 把 nice 当优先级——它是 CFS 权重（core.c:11542），无任何抢占承诺；
2. `getpriority()` 不清 errno 就判断错误——内核返回 rlimit 风格 1..40，glibc 转换后 -1 是合法值（sys.c:282-287）；
3. `setpriority` 传越界值不报错——内核静默钳制（sys.c:232-235），必须读回验证；
4. 同核放两个同级 FIFO——第二个饿死是**设计行为**（task_tick_rt:2652）；
5. 以为 `sched_yield()` 能给低优先级任务让路——RT 的 yield 只排队尾（rt.c:1590）；
6. 满负荷压测出现 50ms 周期性延迟尖峰——先查 `dmesg | grep "RT throttling"`；
7. 绑核后以为独占——负载均衡照样派活，要 isolcpus + nohz_full + IRQ 绕行；
8. RT 服务 fork/exec 脚本不带 `SCHED_RESET_ON_FORK`——脚本继承 FIFO 90 是整机级炸弹（负 nice 同样被重置，core.c:4753-4754）；
9. nice 降权以为跨终端生效——autogroup（v2.6.38+ 默认开）让 nice 只在会话组内有意义；
10. `sched_getaffinity` 读回比设置的小就怀疑 bug——可能是 CPU 热插拔（core.c:8482 交集 active）；
11. `sched_setscheduler(pid, …)` 以为改的是"进程"——pid 是 TID 粒度，多线程要逐线程设（`setpriority(PRIO_PROCESS,…)` 同样是 TID 粒度，sys.c:219）；
12. 想用 nice 给 SCHED_FIFO 任务"再提一档"——对 RT 无效，内核只写 static_prio 不动调度（core.c:7209-7212）；
13. 普通用户把进程切 SCHED_IDLE 当"礼貌让路"——**单向门**：切回按 nice 20 提权审查，RLIMIT_NICE=0 必 EPERM（core.c:7598）；
14. throttling 实验单次读数就下结论——任务起点与 period 相位的对齐让同程序两次跑差出 10%/14%，看数量级不看个位数。

---

## 实验清单

| # | 实验 | 状态 | 所需权限 |
|---|------|------|----------|
| 1 | nice 三 API + getpriority 陷阱 + EACCES 边界（`nice_probe.c`，35.1） | ✅ 实跑（普通用户 + root 对照） | 无 |
| 2 | RR 时间片查询 100ms（`rr_interval.c`，35.2） | ✅ 实跑（双身份对照） | 查询无 / 切 RR 需 root |
| 3 | FIFO 80 设置 + 降级自由 + RESET_ON_FORK 隔代重置（`rt_setup.c`，35.3） | ✅ 实跑（双身份对照） | root |
| 4 | 绑核/空掩码 EINVAL/替换语义（`affinity_pin.c`，35.4） | ✅ 实跑 | 无 |
| 5 | FIFO 99 满负荷 vs OTHER 饿死 + throttling 活口（`fifo_starve.c`，35.6） | ✅ 实跑 | root |
| 6 | RR 轮转 vs FIFO 独占双子对比（`rr_alt.c`，35.6） | ✅ 实跑 | root |
| 7 | RT throttling 配额直击 100ms/1s + dmesg 指纹（`rt_throttle.c`，35.6） | ✅ 实跑（自动恢复 sysctl） | root |
| 8 | CFS 权重比 nice0/nice5 = 3.03:1（预写 3.06，偏差<1%）（`nice_race.c`，35.6） | ✅ 实跑（双身份对照） | 无 |
| 9 | SCHED_IDLE 单向门 + vs nice19 权重比 4.93:1（`idle_weight.c`，35.6） | ✅ 实跑（门=普通用户，比=root） | 分模式 |
| 10 | RESET_ON_FORK 隔代：子重置/孙原样继承（`rof_grand.c`，35.6） | ✅ 实跑 | root |

历史 demo：[`code/t_nice.c`](code/t_nice.c) · [`code/sched_view.c`](code/sched_view.c) · [`code/affinity_demo.c`](code/affinity_demo.c)（早期版本，功能被笔记内嵌 demo 覆盖）。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | nice ∈ [-20,19]：查 `sched_prio_to_weight`（每级 ×1.25），只管 CFS |
| 2 | 用户 RT 1..99 → 内核 prio = 99-rt（反向）；RT 段 0..98 恒压 CFS 段 100..139 |
| 3 | 内核 syscall getpriority 返回 1..40（20-nice），glibc 转回 nice |
| 4 | 非特权降权自由；提权要 RLIMIT_NICE；切 RT 要 RLIMIT_RTPRIO≠0；兜底 CAP_SYS_NICE |
| 5 | FIFO 无时间片（rt.c:2652 直接 return）；RR = FIFO + 100ms（RR_TIMESLICE rt.h:57） |
| 6 | 调度类链 stop > dl > rt > fair > idle，高类整体压制低类 |
| 7 | RT throttling：950ms/1s，超限**整队**冻结，"RT throttling activated" 进 dmesg |
| 8 | RLIMIT_RTTIME：每任务 CPU 累计超限收 SIGKILL（第二道防线） |
| 9 | RESET_ON_FORK：子进程回 OTHER/nice0/rt0，负 nice 也归零，flag 只管一代 |
| 10 | sched_yield：RT 只排队尾，绝不让低优先级 |
| 11 | affinity 掩码先与 cgroup 交集，空交集 EINVAL（core.c:8332） |
| 12 | 完整隔离 = 绑核 + isolcpus + nohz_full + IRQ 绕行（+ rcu_nocbs） |
| 13 | autogroup（默认开）：CFS 先按会话组公平，nice 跨会话无效 |
| 14 | nice 对 RT 任务无效：set_user_nice 只写 static_prio（core.c:7209-7212） |
| 15 | HFT 启动序：mlockall → 预热 → 绑核 → FIFO → RTTIME 兜底 |
| 16 | RT 选任务 O(1)：100bit 位图 `sched_find_first_bit` + 每优先级一链表（rt.c:1769） |
| 17 | SCHED_IDLE 权重 = 3（WEIGHT_IDLEPRIO），是 nice19(15) 的 1/5，实测 4.93:1 |
| 18 | 迁移由最高类的 stopper 线程完成：五步注释 core.c:2497，dequeue→换核→enqueue |

---

## 参考

- Kerrisk · TLPI Ch35（原书 6 节结构：35.1 优先级与 nice / 35.2 RT 概述 / 35.3 RT API 详解 / 35.4 CPU 亲和 / 35.5 总结 / 35.6 练习）
- `man 2 sched_setscheduler` · `man 2 setpriority` · `man 2 sched_setaffinity` · `man 7 sched`
- 内核文档：`Documentation/scheduler/sched-rt.rst` · `isolcpus` 与 cgroup v2 cpuset partition
- 本仓关联：[Ch29 线程简介](../chapter-29-threads-intro/README.md)（每线程调度属性）· [Ch30 线程同步](../chapter-30-thread-synchronization/README.md)（RT 线程的优先级继承）· [Ch33 线程进阶](../chapter-33-threads-further/README.md)（SCHED_RESET_ON_FORK 与线程）· [Ch36 进程资源](../chapter-36-process-resources/README.md)（RLIMIT 三兄弟）· [Ch39 capabilities](../chapter-39-capabilities/README.md)（CAP_SYS_NICE）
