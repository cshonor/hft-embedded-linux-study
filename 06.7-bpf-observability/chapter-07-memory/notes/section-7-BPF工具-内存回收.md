# 7.3 BPF 工具（五）：内存回收与换页 — vmscan / drsnoop / swapin

> 底本：《BPF之巅》第 7 章 内存，7.3.8–7.3.10 节（印刷 p281–287）。内存紧张时内核的三类活动：VM 扫描回收、直接回收阻塞、页换入。

## 7.3.8 vmscan — VM 扫描器耗时（bpftrace）

用 vmscan 跟踪点观察 kswapd 的回收工作。**名字里的 scanner 是历史**：内核早已用 LRU 链表管理活跃/非活跃内存。

每秒输出五列（36-CPU 内存压力生产机的实测）：

```
TIME      S-SLABms  D-RECLAIMms  M-RECLAIMms  KSWAPD  WRITEPAGE
21:30:27       555             ← slab 收缩 555ms
21:30:28        72     49    41            ← 三类回收同时出现
21:30:29        35    454
```

| 列 | 含义 |
|----|------|
| S-SLABms | 收缩 slab（内核缓存）总耗时 |
| **D-RECLAIMms** | **直接回收**总耗时（前台、阻塞分配，**性能问题的信号**） |
| M-RECLAIMms | memcg 回收耗时（cgroup 超限触发） |
| KSWAPD | kswapd 唤醒次数 |
| WRITEPAGE | kswapd 写出页数 |

时间是**所有 CPU 累计** — vmstat/sar 看不到的真实消耗。另有直接回收和 slab 收缩的纳秒直方图（书例：直接回收主峰 1–2ms，slab 收缩主峰 256–512µs）。

**重点关注 D-RECLAIM**：直接回收"不好但必须"，通常造成性能问题；调优方向是 vm sysctl 让**后台回收提前启动**，避免走到前台直接回收。

源码结构（跟踪点 + 双探针计时模板 + interval 输出清零，值得模仿）：

```bash
tracepoint:vmscan:mm_shrink_slab_start { @start_ss[tid] = nsecs; }
tracepoint:vmscan:mm_shrink_slab_end /@start_ss[tid]/
{
    $dur_ss = nsecs - @start_ss[tid];
    @sum_ss = @sum_ss + $dur_ss;
    @shrink_slab_ns = hist($dur_ss);
    delete(@start_ss[tid]);
}
# mm_vmscan_direct_reclaim_begin/end、mm_vmscan_memcg_reclaim_begin/end 同构
tracepoint:vmscan:mm_vmscan_wakeup_kswapd { @count_wk++; }
tracepoint:vmscan:mm_vmscan_writepage    { @count_wp++; }
interval:s:1
{
    time("%H:%M:%S ");
    printf("%10d %12d ...", @sum_ss/1000000, @sum_dr/1000000, ...);
    clear(@sum_ss); ...    # 每秒打印后清零
}
```

## 7.3.9 drsnoop — 直接回收逐事件（BCC）

vmscan 给总量，drsnoop 给**每次**直接回收的受害者与延迟：

```
# drsnoop -T
TIME(s)       PID     COMM   LAT(ms)  PAGES
0.000000000   11266   java      1.72      57
0.004007000   11266   java      3.21      57
0.011856000   11266   java      2.02      43
0.024647000    1209   acpid     6.46      73
```

java 每次直接回收阻塞 1–7ms — **直接量化内存压力对应用延迟的影响**。

- 跟踪 `mm_vmscan_direct_reclaim_begin/end` 跟踪点
- 直接回收是短时集中爆发的低频事件 → 开销可忽略
- 选项：`-T` 时间戳、`-p PID`

## 7.3.10 swapin — 谁在换入（bpftrace）

vmstat 的 si 列只知道全系统换入了多少 KB，**不知道哪个进程受害** — swapin 补上：

```
# vmstat 1（si 列 = 36 KB/s）
# swapin.bt
06:57:44  @[systemd-logind,1354]: 9     ← 9 次 × 4KB = 36KB，对上了
```

书例点睛：作者 SSH 登录变慢，因为 logind 的部分内存被换出，登录过程触发换入等待。

**关键认知**：换入（swapin）在应用**使用**被换出的内存时触发，是**唯一直接影响应用性能的换页指标**；扫描、换出等指标都只是间接信号。

```bash
#!/usr/local/bin/bpftrace
kprobe:swap_readpage
{
    @[comm, pid] = count();
}
interval:s:1 { time(); print(@); clear(@); }
```

swap_readpage() 在触发换入的进程上下文执行 → comm/pid 就是受害者。

## 工具递进关系

```
vmstat si/so → 有换页吗（全系统）
vmscan       → 回收耗时多少（S/D/M 列，谁在忙）
drsnoop      → 直接回收卡了谁、卡多久（逐事件）
swapin       → 换入卡了谁（逐事件）
```

## HFT 关联

- 交易机无 swap → swapin 无用武之地，但 **drsnoop 是核心**：直接回收 = 毫秒级停顿，drsnoop -T 记录每次受害者与延迟，配合 vmscan 的 D-RECLAIM 列做告警线（>0 即告警）
- vmscan 常驻（跟踪点 + 每秒聚合，开销低）：D-RECLAIM 持续非零说明 `vm.min_free_kbytes` 等水位参数需要调
- 三板斧：vmscan 定性（有无压力）→ drsnoop 定位（谁受害）→ pmap/memleak 定源（谁吃光了内存）

## 常见陷阱

1. **只看 si/so 判断换页影响** — 换出（后台）不直接伤应用，换入才伤；且无 swap 系统的痛苦全在直接回收，si/so 永远为 0
2. **把 vmscan 时间当单核耗时** — S/D/M 列是所有 CPU 的累计毫秒
3. **看到 KSWAPD 唤醒就紧张** — kswapd 后台回收是正常机制；要紧张的是 D-RECLAIM 非零
4. **drsnoop 的 LAT 当成应用延迟** — LAT 是该次分配阻塞在回收上的时长，是应用延迟的**增量**而非全部

<details>
<summary>📝 自测题（点击展开）</summary>

1. **vmscan 输出中哪一列最重要？为什么？调优方向是什么？**

   <details>
   <summary>参考答案</summary>

   D-RECLAIMms（直接回收耗时）。直接回收在分配进程前台执行、阻塞分配，是内存压力转化为性能问题的直接路径；S-SLAB/M-RECLAIM 影响较小。调优方向：通过 vm sysctl（如 min_free_kbytes、watermark_scale_factor）让 kswapd 后台回收更早启动，避免滑入直接回收。
   </details>

2. **为什么说换入比换出更能代表性能影响？**

   <details>
   <summary>参考答案</summary>

   换出由 kswapd 后台执行，应用通常无感；换入发生在应用访问被换出页的时刻，CPU 必须停在那里等磁盘读回 — 同步阻塞。所以 swapin 计数是"应用性能受害"的直接计量（书例：logind 被换出导致 SSH 登录慢）。
   </details>

3. **swapin 工具为什么能直接显示受害进程？**

   <details>
   <summary>参考答案</summary>

   kprobe:swap_readpage 在缺页处理上下文中执行，而缺页处理运行在"访问被换出内存"的那个进程上下文里 — 所以 BPF 程序里的 comm/pid 内置变量天然就是受害进程。
   </details>

</details>
