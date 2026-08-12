# 12 个月 HFT 学习 + 项目路线

> 基于现有 Phase 1-6 + P1-P9 项目结构，映射到 12 个月时间线。
> **目标：** 单机 HFT 技术原型（DPDK + 无锁订单簿 + 内存池 + 撮合引擎），仿真环境跑通完整链路。
> **不是：** 可上实盘的生产系统（需要团队多年迭代的容错/风控/合规）。

---

## 时间线总览

```
M1-M2   Phase 1-2   数字逻辑 + C 语言 + 计算机系统基础
M3-M4   Phase 3     用户态系统编程 + C++ 穿插
M5-M6   Phase 4     内核入门 + 内存管理 + 现代补充
M7-M8   Phase 5A    嵌入式支线（ARM + 驱动 + 板级）
M9-M10  Phase 5B    网络栈 → DPDK → 性能观测
M11-M12 Phase 5B+   HFT 工程实践 + 撮合引擎原型
```

---

## 分月规划

### M1：数字逻辑 + C 语言起步（Phase 1-2）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `00` 数字逻辑/CPU 黑盒语义 | P1 CPU 模拟器（Logisim 8-bit） |
| W3-W4 | `01` K&R + C 和指针 精读 | C 基本功过关：指针、内存模型、struct 对齐 |
| W5-W6 | `01` 嵌入式 C 自我修养（GNU C 扩展） | container_of / __attribute__ / 内嵌汇编 |
| W7-W8 | `02` CSAPP 核心章（Ch6 缓存 / Ch9 VM / Ch12 并发） | 缓存行、虚拟内存、线程模型能讲通 |

**过关标准：** 能手写 `container_of`，能解释 cache line 伪共享，P1 + P2.5 完成。

### M2：Shell + malloc + C++ 起步（Phase 2-3）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `01` C 和指针 ch12-18 + 自测题 | P2 Shell + malloc（fork/exec/pipe + 自制 malloc） |
| W3-W4 | `04` C++ M0 Primer 快速过（C 转 C++） | RAII / 引用 / 模板 / STL 基础 |
| W5-W6 | `04` M1 Effective Modern C++ | auto / move / smart ptr / constexpr |
| W7-W8 | `04` M2 并发 + 对象模型 | std::thread / mutex / vtable / 内存模型 |

**过关标准：** C → C++ 切换完成，能写 RAII 风格的并发 HTTP Server。

### M3：用户态系统编程（Phase 3 核心）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W3 | `03` TLPI 文件 IO（Ch4-5）+ 进程（Ch24-28） | open/read/write/mmap/epoll 熟练 |
| W4-W5 | `03` TLPI 信号（Ch20-22）+ 线程（Ch29-31） | P3 HTTP Server（C epoll 版 → C++ RAII 版） |
| W6-W7 | `03` TLPI IPC（Ch43-53）+ 时间（Ch23） | pipe / shm / sem / timer 实操 |
| W8 | `03` P3.5 BusyBox 极简 Linux | 内核编译 + rootfs + QEMU 启动到 shell |

**过关标准：** epoll / mmap / fork / pthread 能徒手写 Demo，P3 + P3.5 完成。

### M4：内核入门 + 内存管理（Phase 4）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `05` LKD：调度 / 中断 / 同步 | 理解 CFS / softirq / spinlock |
| W3 | `05.5` 现代内核 6.x 差异 | folio / MGLRU / EEVDF |
| W4 | `05.6` 内核调试：KASAN / KGDB / Ftrace | P4 内核模块（字符设备 + /proc 统计） |
| W5-W6 | `06` Gorman MM + `06.5` 现代 MM | slab/slub/buddy/hugepage/NUMA |
| W7-W8 | 内核调优实操 | isolcpus / IRQ 亲和 / PREEMPT_RT 配置 |

**过关标准：** 能写可加载内核模块，能解释 page fault → buddy → slab 链路，P4 完成。

### M5：嵌入式支线（Phase 5A）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `07` ARM/AArch64 + `08` U-Boot/内核构建 | P5a QEMU 裸机 UART + P5b U-Boot→kernel→rootfs |
| W3-W4 | `09` 设备驱动 + 设备树 | P5c I2C/SPI 传感器驱动 |
| W5-W6 | `10` 板级工程 + `11` PID 控制 | P5d 传感器融合 + 延迟 p99 统计 |
| W7-W8 | 嵌入式收尾 / 预热网络模块 | P5e PID 姿态控制（可选） |

**过关标准：** 树莓派能从 U-Boot 启动到 shell，写过真实设备树驱动。

### M6：网络编程 + TCP/IP（Phase 5B 前半）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `12` UNP Socket 编程 + PNP muduo | TCP/UDP /非阻塞 / TCP_NODELAY |
| W3-W4 | `13` TCP/IP Illustrated Vol.1 | UDP 组播 / IP 分片 / TCP 拥塞 |
| W5-W6 | `14` 内核网络栈 + `14.5` 现代网络 | sk_buff / NAPI / XDP / io_uring |
| W7-W8 | P6 网络协议分析器 | raw socket 抓包 + 逐层解析 + eBPF |

**过关标准：** 能解释一个包从网卡到用户态的完整路径，P6 完成。

### M7：DPDK + 性能观测（Phase 5B 后半）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `15` DPDK 官方文档 + 深入浅出 DPDK | PMD / rte_mbuf / 零拷贝 / hugepage |
| W3-W4 | P7 DPDK packet forwarder | 收包→转发→统计，绑核 + busy-poll |
| W5-W6 | `16` Systems Performance（Gregg） | perf / flamegraph / NUMA / 网卡调优 |
| W7-W8 | `17` BPF Performance Tools | bpftrace / BCC / XDP 延迟探针 |

**过关标准：** DPDK 收发包跑通，能用 perf + bpftrace 定位热路径延迟，P7 完成。

### M8：HFT 工程基础（Phase 5B 收尾）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | `18` HFT 工程实践 | 无锁队列 / 内存池 / 时间戳 / SPSC ring |
| W3-W4 | `19` 体系结构加深 | cache line / MESI / memory ordering / false sharing |
| W5-W6 | 延迟测量工具链 | RDTSC / histogram / latency p50/p99/p999 |
| W7-W8 | `22` 市场微观结构（Trading and Exchanges） | LOB / 撮合算法 / 订单类型 / 做市商 |

**过关标准：** 手写过无锁 SPSC ring buffer，能用 RDTSC 做纳秒级延迟测量。

### M9：撮合引擎 — 核心数据结构（P8 启动）

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | 订单簿数据结构设计 | 红黑树 + 哈希表的 price-level 结构 |
| W3-W4 | 内存池 + 对象池 | 预分配 / slab 风格 / 不 new/malloc |
| W5-W6 | 撮合逻辑（限价单 + 市价单） | price-time priority / 成交匹配 |
| W7-W8 | 绑核 + hugepage + CPU 隔离 | isolcpus / mlock / NUMA 绑定 |

**过关标准：** 订单簿能正确撮合，内存池零动态分配，绑核跑通。

### M10：撮合引擎 — 行情接入 + 策略接口

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | 行情模拟器（二进制协议） | 自定义二进制行情格式 + UDP 组播发送 |
| W3-W4 | DPDK 行情接收 | 用户态收包 → 解析 → 更新订单簿 |
| W5-W6 | 策略接口设计 | 信号生成 → 风控检查 → 下单 |
| W7-W8 | 回测框架 | 行情回放 + 策略 PnL 统计 |

**过关标准：** 完整链路跑通：收行情 → 更新订单簿 → 生成信号 → 模拟下单 → 统计 PnL。

### M11：撮合引擎 — 延迟优化 + 风控

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | 热路径延迟剖析 | perf 火焰图 + bpftrace 探针 + 逐函数优化 |
| W3-W4 | 缓存优化 | cache line 对齐 / prefetch / false sharing 消除 |
| W5-W6 | 基础风控层 | 仓位上限 / 重复下单检测 / 异常行情熔断 |
| W7-W8 | 日志 + 监控 | 低延迟日志（lock-free）+ 实时延迟监控 |

**过关标准：** 端到端延迟（收行情到发单）有量化数据，风控覆盖基本异常路径。

### M12：压测 + 文档 + 总结

| 周 | 内容 | 产出 |
|----|------|------|
| W1-W2 | 百万级行情回放压测 | 稳定性测试 + 内存泄漏检测 + 延迟分布 |
| W3-W4 | 边界测试 | 丢包 / 乱序 / 空行情 / 暴涨暴跌场景 |
| W5-W6 | 架构文档 | 系统设计图 + 延迟 benchmark + 技术选型说明 |
| W7-W8 | 求职准备 | 项目讲解话术 + 代码 walkthrough + 技术亮点提炼 |

**过关标准：** P8 撮合引擎原型完成，有完整文档和 benchmark 数据，能 15 分钟讲清楚架构。

---

## 原型 vs 生产系统（边界认知）

| 维度 | 本路线产出（原型） | 机构级生产系统 |
|------|-------------------|---------------|
| 行情接入 | 模拟器 / WebSocket | 交易所专线 UDP 组播 + 二进制协议 |
| 风控 | 基础仓位/重复检测 | 成千上万边界 + 实时熔断 + 多层校验 |
| 容错 | 单机，crash 即停 | 双机热备 + 故障切换 + 状态恢复 |
| 时间同步 | RDTSC / clock_gettime | 硬件 PTP + 多源比对 + 坏数据剔除 |
| 合规 | 无 | 交易所认证 + 审计日志 + 监管对接 |
| 压测 | 百万级回放 | 千万级 + 线上持续 + 概率性 bug 复现 |

**结论：** 原型的价值在于**学习全链路技术 + 求职展示**，不是直接上实盘。

---

## 关键里程碑检查点

| 月份 | 检查项 | 如果没达到 |
|------|--------|-----------|
| M2 末 | C/C++ 基本功过关，P1+P2+P2.5 完成 | 延后 Phase 3，不硬冲 |
| M4 末 | 能写内核模块，理解 MM 链路 | 延后网络模块 |
| M7 末 | DPDK 收发包跑通，perf/bpftrace 熟练 | 不开撮合引擎 |
| M10 末 | 撮合引擎完整链路跑通 | 砍风控复杂度，保链路完整 |
| M12 末 | 原型完成 + 文档 + benchmark | 即使简化也要有可展示的交付 |

---

## 注意事项

1. **不跳阶段**：Phase 1-2 没过不要冲内核/DPDK
2. **时间不均分**：M1-M2 基础可能超时，正常，基础不牢后面全崩
3. **嵌入式支线可压缩**：如果 M5 时间紧，P5c-P5e 可选做，保 P5a-P5b
4. **每个 P 都要动手写代码**：不只是读模块笔记
5. **延迟数据要量化**：不靠感觉说"快"，靠 RDTSC + histogram
