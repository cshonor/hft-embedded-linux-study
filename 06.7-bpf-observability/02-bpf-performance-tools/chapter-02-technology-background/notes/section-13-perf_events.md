# 2.13 perf_events（BPF 工具的事件汇聚层）

> 底本：《BPF之巅》第 2 章技术背景，2.13 节（印刷 p69–70）

## 是什么

perf_events 是 perf(1) 命令所依赖的**采样与跟踪机制**，2009 年随 Linux 2.6.31 合入。

## BPF 工具与 perf_events 的关系（演进三步）

1. BCC/bpftrace 先把 perf_events **用作环形缓冲区**（内核→用户态传数据）
2. 又通过它**增加了对 PMC 的支持**
3. 现在**通过 perf_event_open() 观测所有事件**（kprobe/uprobe/tracepoint/USDT/PMC 统一挂载）

→ perf_events 实际成了 BPF 跟踪工具的事件汇聚层/统一入口。

这个"汇聚"的架构意义值得展开：perf_event_open(2) 把异构事件源统一成一种内核对象（perf event fd），BPF 工具只需要会一种挂载协议：

```text
                    perf_event_open(2)
                         │ 统一接口
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
   kprobe/uprobe    tracepoint/USDT     PMC 溢出
        └────────────────┼────────────────┘
                         ▼
              perf event（每 CPU 一个环形缓冲）
                         ▲ 事件写入
              挂在该 event 上的 BPF 程序
                         ▲
              bpf(BPF_PROG_LOAD) + ioctl(PERF_EVENT_IOC_SET_BPF)
```

对比"每种事件源一套独立 API"的世界：工具开发者要为每个源写不同的挂载代码。汇聚层把这个成本摊平——这也是 BCC 70+ 工具能快速长出来的生态学原因之一。

> 补充（书成之后的演进）：现代 libbpf 引入了 **BPF link**（attach 关系成为一等对象）与 perf_event 之外的新挂载通道（如 fentry 直接 link），perf_events 不再是唯一汇聚层，但"统一事件对象"的设计思想被继承。

## perf(1) 本身也是 BPF 前端

perf(1) 开发了使用 BPF 的接口，成为又一个 BPF 跟踪器。与 BCC/bpftrace 不同：**perf(1) 代码在 Linux 内核源码树中，是唯一内置的 BPF 前端**（无需额外安装）。其 BPF 功能仍在开发、使用尚不便利（附录 D 有 perf+BPF 例子）。

## HFT 关联

- 理解"一切事件经由 perf_event_open()"意味着：排障时用 `perf list` / `bpftool perf show` 就能盘点系统上所有观测点（含别人挂的），避免观测互相踩踏。
- 生产交易机用 perf(1) 内置 BPF 能力可少装一个包（BCC 全家桶很重），轻量采集场景（如 PMC 采样画火焰图）perf(1) 一把梭。
- 交易机的"观测点台账"可以建立在 perf event 之上：常驻探针统一用 `bpftool perf show` 导出清单，变更评审时核对——谁挂的、挂哪了、事件率多少，一目了然。

## 陷阱

- 多个工具同时经 perf_events 挂同一事件时输出会互相影响；排查"数据怪异"先查 `bpftool perf show`。
- perf(1) 在内核树中 → 版本与内核强绑定，跨机器迁移脚本时注意版本差异。
- perf_event_open 需要权限（CAP_PERFMON / paranoid 设置）：容器化交易服务里跑 BCC 工具常在这里失败，报"Operation not permitted"——不是 BPF 的问题，是事件汇聚层的准入问题。

## 自测

<details>
<summary>1. BCC/bpftrace 使用 perf_events 的三个阶段是什么？</summary>

环形缓冲区 → PMC 支持 → 通过 perf_event_open() 观测所有事件（统一事件汇聚层）。
</details>

<details>
<summary>2. 为什么说 perf(1) 是唯一内置的 BPF 前端？</summary>

它的代码位于 Linux 内核源码树中，随内核发布；BCC/bpftrace 都是外部项目。
</details>

<details>
<summary>3. "统一事件对象"的设计给工具生态带来什么好处？BPF link 出现后这一思想如何延续？</summary>

工具开发者只需实现一种挂载协议（perf_event_open + ioctl SET_BPF），新事件源接入即被全部工具免费获得——摊平了每源适配成本，支撑了 BCC 工具数量的爆发。BPF link 把"attach 关系"提升为一等内核对象（可 pin、可原子替换），依然是为异构事件源提供统一抽象，只是从"fd 副作用"升级成"独立对象"。
</details>

<details>
<summary>4. 容器里跑 bpftrace 报 Operation not permitted，最可能的检查点是什么？</summary>

perf_event_open 的权限准入：kernel.perf_event_paranoid 设置与容器的 CAP_PERFMON capability——汇聚层不让进门，后面的 BPF 加载根本走不到。
</details>
