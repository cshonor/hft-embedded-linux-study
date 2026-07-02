## 6.1–6.3 CPU 模型与核心概念

### 硬件层级

```
Socket (Processor)
  └── Core × N
        └── Hardware Thread × 1~2 (SMT / 超线程)
              └── L1I / L1D → L2 → L3 (共享)
                    └── 访问 DRAM（最慢）
```

| 概念 | 含义 | HFT |
|------|------|-----|
| **Processor / Socket** | 物理 CPU 封装 | 双路服务器 = 2 sockets |
| **Core** | 独立执行单元 | **1 热线程 : 1 物理核** 常见 |
| **Hardware thread** | 逻辑 CPU（SMT） | 与同核另一线程争资源 — 热路径避免共享 |
| **Run Queue** | 就绪等 CPU 的线程队列 | 长度 > 0 持续 = **调度饱和度** |

→ [02-Hennessy](../../../02-Computer-Architecture-6th/) 流水线与 cache · [01-CSAPP Ch6](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/)

### 时钟、流水线、IPC / CPI

| 指标 | 定义 | 解读 |
|------|------|------|
| **Clock Rate** | 时钟频率（GHz） | 同代 CPU 频率差有限，别唯频率论 |
| **Pipeline** | 指令多级流水 | 分支预测失败、依赖链会冒泡 |
| **IPC** | Instructions Per Cycle | 高 → 执行单元充实 |
| **CPI** | Cycles Per Instruction = 1/IPC | 高 → 常在 stall |
| **Stall cycles** | 流水线停顿 | 多因 **cache miss、TLB miss、等内存** |

**经验：**

```bash
perf stat -e cycles,instructions,cache-misses,cache-references -- sleep 1
# 看 IPC = instructions / cycles
```

- IPC 接近理论峰值 → CPU 算力瓶颈或指令本身重
- IPC 明显偏低 + cache-misses 高 → **先查内存布局 / 数据结构**（Ch 7），而非盲目 `-O3`

### 关键指标：利用率、饱和度、User/Kernel

| 指标 | 看什么 | 工具 |
|------|--------|------|
| **Utilization** | 非 idle 时间占比 | `mpstat`、`/proc/stat` |
| **Saturation** | run queue 长度、调度延迟 | `vmstat r`、BPF `runqlat`/`runqlen`、PSI |
| **User time** | 用户态算力 | 策略、解码 |
| **Kernel time** | 内核态 | syscall、协议栈、驱动 |
| **Steal time** | 虚拟化被宿主机偷走 | 云环境 — HFT 共置尽量为 0 |
| **Priority inversion** | 高优先级被低优先级间接阻塞 | RT 线程 + 锁 — 用优先级继承或隔离 |

**HFT 警示：**

- **单核 100% user** 不一定是好事 — 可能是 **spin 忙等**；要结合 run queue 与 off-CPU。
- **kernel % 高** 在行情机常见 — 查 softirq、网络栈、是否该上 DPDK。

→ Ch 2 [USE 方法](../../chapter-02-methodologies/) · [附录 A CPU 项](../../appendix-A-USE方法Linux.md)

---


---

← [本章导读](../README.md)
