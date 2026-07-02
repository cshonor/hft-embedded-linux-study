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

→ 亲和性 syscall：[section-6](./section-6-调度相关系统调用.md) · NUMA：[07 Gorman](../../../05-Linux-Virtual-Memory-Manager/)

---

← [4. 核心函数](./section-4-调度算法与核心函数.md) · 下一节 [6. 系统调用](./section-6-调度相关系统调用.md)
