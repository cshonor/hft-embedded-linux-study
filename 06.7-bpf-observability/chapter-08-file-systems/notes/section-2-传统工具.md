# 8.2 传统工具

> 底本：《BPF之巅》第 8 章 文件系统，8.2 节（印刷 p300–303）

| 工具 | 类型 | 用途 | 关键注意 |
|---|---|---|---|
| df | 容量 | 各文件系统空间使用 | Use% >90% 性能下降（空闲块碎片化） |
| mount | 配置 | 挂载点与选项 | `noatime` 是常见低风险优化 |
| strace | 系统调用跟踪 | 逐个看 open/read/write | **ptrace 机制可让目标掉到 1% 性能，生产禁用** |
| perf | 采样/跟踪 | perf trace / perf stat / perf record | 见下方自反馈陷阱 |
| fatrace | 文件访问 | fanotify API 全局文件事件 | 实测 67% CPU；对比 opensnoop 仅 1.1% |

## df

```
# df -h
Filesystem  Size  Used Avail Use% Mounted on
/dev/sda1    99G   91G  2.7G  98% /
```

Use% 高不仅意味着空间不够，还意味着**性能退化**：接近满的 ext4 空闲块分散，多块分配（extent）退化，写入变慢且碎片化。

## mount

关注挂载选项：`noatime`（不更新访问时间戳）可减少元数据写；`relatime` 是折中默认。

## strace

ptrace 每次系统调用都陷入 tracer，开销巨大——本书反复强调的生产禁忌。替代：perf trace（高效版 strace）、BPF 的 opensnoop/statsnoop/scread。

## perf

- `perf trace`：基于 perf 的高效 strace 替代。
- `perf stat -e 'ext4:*'`：按文件系统跟踪点计数。
- **perf record 自反馈循环陷阱（书例）**：对文件系统写事件做 perf record，事件本身产生样本 → 样本写入 perf.data 又触发文件系统写 → 更多事件。书例最终产生 **1.3GB 数据文件 / 1400 万样本**，大部分在测 perf 自己。BPF 在内核态聚合（map 里只存统计量）天然避免此问题——这正是 bpftrace 单行优于 perf record 的场景。

## fatrace

基于 fanotify 的文件访问通知工具。开销对比实测（跟踪同一负载）：

| 工具 | CPU 开销 |
|---|---|
| fatrace | 67% |
| opensnoop (BCC) | 1.1% |

传统 fanotify 路径每次事件都要到用户态处理；BPF 在内核过滤聚合。

## HFT 关联

- 交易机上 strace/fatrace 这类高开销跟踪器一律禁用；需要看文件行为用 opensnoop/filetop。
- 磁盘使用率监控阈值设 90%，不只是容量告警，也是性能告警。

<details>
<summary>自测</summary>

1. 为什么 Use% 接近 100% 时文件系统写入性能会下降？
   <details><summary>答案</summary>接近满的文件系统空闲块高度碎片化，分配器找不到物理连续的空闲区，extent（多块分配）退化成零散小块——每次写入的块分配搜索变慢，且产生碎片化布局让后续顺序读写也变慢。容量问题和性能问题是同一个根源的两个面。</details>

2. perf record 跟踪文件系统写事件会产生什么问题？BPF 为什么没有？
   <details><summary>答案</summary>自反馈循环：记录写事件产生样本 → 样本写入 perf.data 本身是文件系统写 → 触发更多事件 → 更多样本（书例滚到 1.3GB / 1400 万样本，大部分在测 perf 自己）。BPF 在内核态 map 里聚合，只存统计量不落盘，输出是 Ctrl-C 后的一次性摘要——没有"观测产生被观测事件"的回路。</details>

3. strace 和 opensnoop 测同样的事件，开销差几个数量级？原因是什么？
   <details><summary>答案</summary>约两个数量级（书内同负载实测参照：fatrace 67% vs opensnoop 1.1%，strace 的 ptrace 双停机制更重）。原因：strace 基于 ptrace——每次系统调用目标进程都被停下两次（入口+出口）、信号送 tracer、上下文切换全程参与；opensnoop 是 tracepoint 上挂 BPF，事件在内核态处理完才回用户态打印，目标进程不感知。</details>
</details>
