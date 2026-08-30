# 2.14 小结（第 2 章技术组件全景 + 全章坑点 + 自测）

> 底本：《BPF之巅》第 2 章技术背景，2.14 节（印刷 p70）

## 原书小结

BPF 性能工具用到的技术：扩展版 BPF（虚拟机/验证器/map）、内核态动态插桩（kprobes）、用户态动态插桩（uprobes）、内核态静态跟踪（跟踪点）、用户态静态跟踪（USDT/动态 USDT）、perf_events。调用栈回溯靠帧指针和 ORC，可视化靠火焰图。

## 全章技术地图（一图流）

```
事件源（谁触发）                    BPF 程序（做什么）         输出（怎么呈现）
─────────────────────────          ─────────────────          ─────────────────
kprobes/kretprobes   [内核/动态]    验证器校验的字节码          map（内核聚合）
uprobes/uretprobes   [用户/动态]    + helper（98 个）          perf_events 环形缓冲
tracepoints          [内核/静态]    + 512B 栈/100 万指令       用户态前端（BCC/bpftrace/perf）
USDT/动态USDT        [用户/静态]    栈回溯：FP / ORC           火焰图/直方图/表格
PMC                  [硬件]         （LBR/DWARF 补充）         bpftool 检视
perf_events          [汇聚层]
```

## 选型决策树

1. 有 tracepoint / USDT 吗？→ 有，用静态（稳定+零禁用开销）
2. 没有 → kprobe/uprobe 动态（先确认事件频率不高）
3. 事件每秒百万级（malloc/free 类）→ 放弃逐事件跟踪，找低频替代事件或 PMC 采样
4. 7×24 常挂 → BPF_RAW_TRACEPOINT（4.17+）
5. 跨内核版本分发工具 → CO-RE + libbpf

## 全章坑点表（HFT 视角）

| # | 坑 | 后果 | 对策 |
|---|---|---|---|
| 1 | 无帧指针的二进制抓栈 | 全是 [unknown] | 关键服务 -fno-omit-frame-pointer 编译 |
| 2 | uprobe 挂 malloc/free | 目标慢 10 倍+ | 换低频事件/USDT |
| 3 | 非 per-CPU map 高并发计数 | 丢失更新、数据失真 | per-CPU map / 原子操作 |
| 4 | 火焰图 X 轴当时间读 | 误判执行顺序 | 记住 X 是字母序 |
| 5 | 5.3 前内核写循环 | 验证器拒绝 | 展开或尾调用 |
| 6 | 虚机里 PMC 全 0 | 误以为无硬件瓶颈 | 部署前验证 perf stat |
| 7 | kprobe 挂被内联函数 | attach 失败或静默丢失 | 换相邻函数/tracepoint |
| 8 | USDT 发布版没开编译开关 | 二进制无探针 | CI 断言 readelf -n |

## 自测

<details>
<summary>1. 用一句话概括第 2 章的主线。</summary>

一切 BPF 性能工具 = （静态或动态、内核或用户或硬件的）事件源 × 内核中运行的 eBPF 程序 × map/环形缓冲输出 × 前端工具呈现；选型纪律是"优先稳定静态、避开高频动态"。
</details>

<details>
<summary>2. 你的交易服务延迟尖刺，每秒百万次调用，想看"哪个函数最耗内核时间"，第 2 章技术怎么组合？</summary>

不逐事件跟踪：PMC 溢出采样（99Hz）触发 BPF 抓帧指针栈 → 内核聚合直方图 → off-CPU/on-CPU 火焰图定位宽塔。
</details>

## 交叉引用

- 前一章：[chapter-01-introduction](../../chapter-01-introduction/README.md)
- 下一章：[chapter-03-performance-analysis](../../chapter-03-performance-analysis/README.md)（60 秒分析与 USE 方法论将把本章技术落成清单）
- 全书目录：[BOOK-TOC.md](../../BOOK-TOC.md)
