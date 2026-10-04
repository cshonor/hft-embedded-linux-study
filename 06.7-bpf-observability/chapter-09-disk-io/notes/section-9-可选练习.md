# 9. 可选练习（9.5）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.5 节（印刷 p409）

均可用 bpftrace / BCC 完成：

1. 修改 biolatency(8)：输出**线性**直方图，0–100ms、每 1ms 一档（`lhist(…, 0, 100000, 1000)` 微秒口径）
2. 修改 biolatency(8)：**每秒打印一次**直方图（`interval:s:1` + clear）
3. 开发按 **CPU** 统计磁盘 I/O 完成事件的工具，检查中断是否均摊到所有 CPU（也用线性直方图）
4. 开发类 biosnoop(8) 工具：CSV 输出到文件，仅三字段：完成时长、方向（R/W）、延迟(ms)
5. 保存第 4 题工具 2 分钟输出，画**散点图**：红=读、蓝=写
6. 保存第 2 题工具 2 分钟输出，画**延迟热力图**（可用 awk 把计数栏转成 HTML 表格行、按值选背景色）
7. 用 **block 跟踪点**重写 biosnoop(8)——即 BCC 仓库的 biolatency-tp/biosnoop-tp 思路：`block_rq_issue`+`block_rq_complete`，键 `[dev, sector]`，注意并行同扇区假设
8. 修改 seeksize(8)：展示**完成事件**测量的设备实际寻址距离（与 biopattern 的 issue/complete 之分同理）
9. 开发统计磁盘 I/O **超时**的工具：block 跟踪点 + `BLK_STS_TIMEOUT` 值（参考 bioerr(8)）
10. （进阶，**作者亦未解决**）开发展示块 I/O **合并长度**直方图的工具

## 做题提示

- 第 1/2 题是 `hist()` vs `lhist()` 与 `interval` 的练习——线性直方图适合已知值域（如 SLA 上限 100ms）
- 第 3 题关联中断亲和性：blk-mq 完成中断集中在少数 CPU 会造成软中断热点
- 第 7 题是 kprobe 版与跟踪点版的对照实验，正好体会 9.3.1 讲的"键选择"差异
- 第 5/6 题练的是"文本输出 → 可视化"流水线，散点图看个例、热力图看密度，各自回答不同问题

## 参考骨架（自测后再看）

题 1/2（biolatency 改造——lhist + 每秒打印）：

```awk
kprobe:blk_account_io_start  { @ts[arg0] = nsecs; }
kprobe:blk_account_io_done   /@ts[arg0]/ {
    // 线性直方图：0–100ms、1ms 一档（µs 口径）——SLA 已知值域时比 log2 桶好读
    @ms = lhist((nsecs - @ts[arg0]) / 1000, 0, 100000, 1000);
    delete(@ts[arg0]);
}
interval:s:1 { print(@ms); clear(@ms); }   // 每秒打一次并清零 = 时间序列化
```

题 3（按 CPU 统计完成事件——中断亲和检查）：

```awk
tracepoint:block:block_rq_complete { @[cpu] = count(); }
// 读法：完成中断集中在少数 CPU = 软中断热点核；配 /proc/irq/*/smp_affinity
// 与 ch6 的 IRQ affinity 检查呼应（交易机上这些核不该是策略核）
```

题 7（跟踪点版 biosnoop——体会键选择差异）：

```awk
tracepoint:block:block_rq_issue    { @ts[args->dev, args->sector] = nsecs; }
tracepoint:block:block_rq_complete /@ts[args->dev, args->sector]/ {
    printf("%dms\n", (nsecs - @ts[args->dev, args->sector]) / 1000000);
    delete(@ts[args->dev, args->sector]);
}
// 跟踪点 format 里没有 request 指针 → 只能用 [dev,sector] 替代键，
// 并行同扇区 I/O（镜像/校验）会配对串味——这就是 kprobe 版更稳的原因
```

题 9（超时统计——bioerr 的特化）：

```awk
tracepoint:block:block_rq_complete /args->error == -124/ {   // BLK_STS_TIMEOUT→ETIMEDOUT
    @[args->dev] = count();
}
// 错误码映射走 blk_status_to_errno：BLK_STS_TIMEOUT=15 → ETIMEDOUT(110→-110)；
// 精确值以所测内核 include/linux/blk_types.h 为准（写工具前先核对，别凭记忆）
```

## 与第 8 章练习的衔接

第 8 章练文件系统层（页缓存命中率、ext4 过滤），本章练块层；两者结合即可完成"应用 read() → 页缓存 miss → 块 I/O → 设备完成"的全链路延迟拆解实验。
