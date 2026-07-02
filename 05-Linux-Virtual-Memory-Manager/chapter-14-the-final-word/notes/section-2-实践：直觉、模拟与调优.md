# Ch 14 §2 实践：直觉、模拟与调优

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 2. 实践：直觉、模拟与调优

理论不够用时，内核开发者靠：

| 手段 | 作用 |
|------|------|
| **直觉 (intuition)** | 指导 **先做什么结构** |
| **Workload 模拟** | 在 **特定负载** 下验证算法（如 page replacement） |
| **管理员调优** | **`sysctl`、swap、overcommit、cgroup** — 适应 **不同部署** |

**页面替换** 是研究最多的领域之一，却 **只在某些 workload 下可证「好」** — **算法 + 运维** 缺一不可。

→ 对应本书 **Ch 10 LRU/active-inactive** — 工程折中，非教科书 LRU。

---
