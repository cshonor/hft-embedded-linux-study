# 5. 异常与泄漏排查

### `oomkill`

追踪 **OOM kill 事件**：

| 输出信息 | 价值 |
|----------|------|
| 被杀进程 | 受害者 |
| **触发 OOM 的进程** | 真凶（不一定是受害者） |
| 当时 load average | 系统整体压力 |

```bash
sudo oomkill-bpfcc
```

**HFT：** 共置机某策略把内存打满 → 先 `oomkill` 留证，再 `memleak` 查泄漏。

### `memleak` — 内存泄漏 🔴（本章核心）

追踪 **分配/释放**（如 `malloc`/`free`、`kmalloc`/`kfree`），在采集窗口结束时打印 **仍未释放** 的分配及其 **用户/内核栈**。

```bash
# 用户态 libc 分配（需 uprobes，有开销）
sudo memleak-bpfcc -p $(pidof myapp) -t 60

# 内核分配（示例）
sudo memleak-bpfcc -k -t 120
```

| 注意 | 说明 |
|------|------|
| **开销** | 每次 alloc/free 插桩 — **勿长期挂在热路径** |
| **适用** | 泄漏排查、长时间 RSS 爬升 |
| **HFT** | 开发/压测环境用；生产仅短窗口、限 PID |

**原理直觉：** 记录分配指针 → free 时删除 → 结束时 map 里剩的就是 **疑似泄漏** + 分配栈。

---
