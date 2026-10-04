# 3. BPF 工具：延迟与事件跟踪（biolatency / biosnoop）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.3.1–9.3.2 节（印刷 p383–392）

## biolatency 🔴（BCC / BT）

块 I/O 延迟直方图，**本章第一入口工具**。默认 kprobe `blk_account_io_start`（起）/ `blk_account_io_done`（止）。

典型输出呈**双峰**：128–2047µs 一簇 + 4–32ms 一簇。

- **Netflix 生产案例**：云数据库实例延迟双峰 → 用 biolatency 区分好坏主机，据此**剔除坏主机、改用更大实例**的策略——不是调参数，是用数据改运维决策

选项（逐步细化）：

| 选项 | 作用 |
|------|------|
| `-Q` | 统计**排队时间**（等待时长）而非总时长 |
| `-D` | 按磁盘分直方图 |
| `-F` | 按 I/O 类型（rwbs）分桶：Read / Sync-Write / Flush / Metadata-Read 各自一张直方图 |

实现要点：**以 request 结构体指针地址（arg0）为哈希键**配对起止——因为 I/O 可能在与发起者不同的 CPU/线程上完成，不能用 pid/comm 做键。跟踪点版本（biolatency-tp.bt，见 9.5 练习）改用 `[dev, sector]` 做键，但隐含"无并行同扇区 I/O"的假设。

## biosnoop（BCC）

逐 I/O 事件输出：`TIME / DISK / T / SECTOR / BYTES / LAT(ms)`。

两个进阶用法：

1. **排队证据**：连续递增的扇区 + 几乎同时的完成时间 → 这些 I/O 在设备队列里排队了（发布被串行拖慢）
2. **QUE(ms) 列**：含排队时间。书中案例：CFQ 调度器 + USB 盘，写排队**超 2 秒**——总延迟不可怕时排队可能已不可接受

实现要点：`blk_account_io_start` 处于进程上下文（能拿 pid/comm），完成时不在——所以发起时**缓存 pid/comm**，完成时取出打印。与 biolatency 同理：request 指针做跨事件键。

> biolatency（看分布）→ 有问题 → biosnoop（看个例找模式）是标准两步。

## HFT 关联

- 交易日志盘：`biolatency -F` 先分清 Sync-Write 和后台写回/flush 的延迟——撮合路径只关心前者
- 延迟毛刺归因：biosnoop 的 QUE 列区分"设备慢"（服务时长）vs"排队"（调度器/邻居 I/O）

## 常见陷阱

- 在高 IOPS 系统长期挂 biosnoop（逐事件 printf 有开销）；先 biolatency 聚合，确认有必要再逐事件
- 用 pid 做 I/O 起止配对键——发起与完成常在不同上下文，键会丢
- 跟踪点版 `[dev,sector]` 键在并行同扇区 I/O（如校验/镜像）下配对出错

<details>
<summary>自测题</summary>

1. biolatency 默认用哪两个 kprobe？为什么用 request 指针做哈希键？
   <details><summary>答案</summary>`blk_account_io_start`（起）/ `blk_account_io_done`（止）。因为 I/O 的发起与完成常发生在**不同 CPU/线程上下文**（中断上下文完成、kworker 写回发起），pid/comm 在两侧不一致；request 结构体指针在请求生命周期内恒定唯一——它是唯一贯穿起止的标识。（与 ch05 的 /@start[tid]/ 计时模板同类思路，但键从 tid 换成了更本质的请求指针。）</details>

2. -Q 和 -F 分别改变什么统计口径？
   <details><summary>答案</summary>-Q 把统计对象从"总请求时长"换成"排队等待时长"（发布前那段）——用于区分设备慢还是队列挤；-F 按 rwbs（Read/Sync-Write/Flush/Metadata）各出一张直方图——用于把延迟归到 I/O 类型（比如只有 Sync-Write 慢 → 排查 fsync 路径）。</details>

3. biosnoop 中什么样的输出模式是排队证据？
   <details><summary>答案</summary>扇区连续递增的一串 I/O，完成时间却几乎同时——说明它们被攒在设备队列里串行服务，发布时刻被前面的 I/O 拖住（QUE 列大而服务时长正常）。对照：真设备慢是每个 I/O 的服务时长都大。</details>
</details>
