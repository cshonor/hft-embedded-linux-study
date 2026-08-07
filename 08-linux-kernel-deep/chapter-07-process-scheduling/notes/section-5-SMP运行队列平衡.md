## 5. 多处理器的运行队列平衡 (SMP)

> SMP / 超线程 / NUMA 下，避免某些 CPU 过载、其他 CPU 空闲

---

### 一、调度域 (Scheduling Domains)

- CPU 组织成 **树状层次** 的调度域  
- 每个域再分多个 **组（Groups）**  
- 反映 **拓扑**：同一 socket、NUMA 节点、物理核 vs 逻辑核 等

---

### 二、负载均衡

| 函数 | 作用 |
|------|------|
| **`rebalance_tick()`** | 定期触发（tick 路径） |
| **`load_balance()`** | 检查调度域是否 **失衡** |
| **`move_tasks()`** | 从最忙 runqueue **迁移** 进程到本地 runqueue |

**HFT 注意：** 生产环境常 **`sched_setaffinity` 绑核** — 刻意 **避免** 迁移带来的 cache 失效与抖动。

→ 亲和性 syscall：[section-6](./section-6-调度相关系统调用.md) · NUMA：[07 Gorman](../../../09-linux-mm/)

### 常见陷阱

1. 以为负载均衡是即时的——负载均衡周期性执行（tick 触发），有迁移延迟
2. 混淆 load balance 和 task migration——load balance 在 CPU 间迁移任务，HFT 要避免迁移（cache miss）
3. 以为 `sched_setaffinity()` 就能保证不被迁移——RT 线程可以，CFS 线程可能在负载均衡时被迁移到同 affinity 集合内的其他核

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** SMP 负载均衡的触发时机和策略？

<details><summary>答案</summary>

触发：① `scheduler_tick()`（周期性，每 ~1ms 检查）。② CPU idle 时主动拉任务（idle balance）。③ `kick_offline_cpu()` 等特殊路径。策略：① domain 层级（SMT → MC → DIE → NUMA），从低到高检查不平衡。② 计算各 CPU 的 `load`（基于 `sched_entity` 权重和运行时间）。③ 如果不平衡度超阈值，从最忙的 CPU 迁移任务到最闲的 CPU。

</details>

**Q2.** HFT 为什么要避免任务在 CPU 间迁移？

<details><summary>答案</summary>

迁移导致：① L1/L2/L3 cache 全部 miss（冷启动延迟）。② TLB 刷新（`switch_mm()` 写 `CR3`）。③ NUMA 跨节点访问延迟（local → remote 多 ~100ns）。一次迁移的延迟惩罚可达 10-100us。避免方法：`sched_setaffinity` 绑定单核 + `isolcpus` 隔离 + `numactl --membind` 绑定本地内存。

</details>

**Q3.** `isolcpus` 和 `cpuset` cgroup 有什么区别？

<details><summary>答案</summary>

`isolcpus=N`：启动参数，从调度器可运行 CPU 集合中移除 N 号核，普通任务不会调度到 N。需要手动 `taskset` 或 `sched_setaffinity` 把 RT 线程放到 N。`cpuset` cgroup：运行时配置，创建 cgroup 设置 `cpuset.cpus=N`，把任务加入该 cgroup。`isolcpus` 更彻底（连 kworker/RCU 都不走），`cpuset` 更灵活（可运行时调整）。

</details>

</details>

---

← [4. 核心函数](./section-4-调度算法与核心函数.md) · 下一节 [6. 系统调用](./section-6-调度相关系统调用.md)
