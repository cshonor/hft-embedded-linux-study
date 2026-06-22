## 16.1.7–16.1.8 动态追踪与结论

### 动态追踪（Tracing）

统计与 PMC **仍不够** — 需要 **谁、哪条路径、何时**：

| 工具 | 案例中的角色 |
|------|--------------|
| **`perf record -g`** | CPU 热点是否迁移到其他函数 |
| **`perf trace` / BPF** | syscall 路径是否变短 |
| **Ftrace function_graph** | 内核路径耗时是否变 |
| **BCC / bpftrace** | runqlat、offcputime、tcpretrans 等 **专项** |

**原则（Ch 4）：**

- 追踪 **开销高** — 窄范围、短时长、带假设
- 要 **栈** — 否则只知「快了」，不知「哪条路径快了」

```bash
# 案例式组合（示意）
perf stat ...                    # 16.1.5–6
perf record -F 99 -g ...         # 16.1.7
sudo runqlat-bpfcc 10            # 调度是否仍饱和
sudo offcputime-bpfcc -p PID 20  # 阻塞栈是否消失
```

→ Ch 13–15 · [附录 C](../../appendix-C-bpftrace单行命令.md)

### 结论（Conclusion）— Drill-Down 拼图

**案例收官：** 把线索 **串成因果链** — 能回答：

1. **什么** 变了（配置/负载/代码/环境）？
2. **哪一层** 受益（CPU cache / 调度 / 网络 / 磁盘）？
3. **证据** 是什么（PMC、trace、配置 diff）？
4. **可行动** 项 — 保留 win、写 runbook、还是 **其实是假象**？

**Unexplained Win 的典型结局类型（学习框架，非剧透具体 Netflix 细节）：**

| 类型 | 教训 |
|------|------|
| 负载/邻居变化 | 云/multi-tenant 对比必须 **同条件** |
| 配置 drift | 静态配置审计（16.1.4） |
| 缓存/预热 | 区分 cold vs steady state（Ch 12） |
| 真实代码/栈优化 | PMC + profile **可复现** |
| 测量 artifact | 指标口径、窗口、采样率 |

---


---

← [本章导读](../README.md)
