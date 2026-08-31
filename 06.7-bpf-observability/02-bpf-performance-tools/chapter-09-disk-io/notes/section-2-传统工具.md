# 2. 传统工具（9.2）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.2 节（印刷 p374–383）

## 9.2.1 iostat

```bash
iostat -dxz 1     # -d 扩展设备 -x 扩展统计 -z 跳过空闲设备
```

关键列：

| 列 | 含义 | 要点 |
|----|------|------|
| `await` | **最重要**：平均请求时长（ms） | 但只是平均，掩盖分布 |
| `r/s`、`w/s` | 每秒读写 IOPS | |
| `rkB/s`、`wkB/s` | 吞吐 | |
| `%util` | 繁忙度 | 只测"忙"，NVMe/多设备上不可作饱和指标 |

书中生产案例：`xvdb` 读取吞吐超平时 16 倍，但应用性能**符合预期**——真问题是**文件大于页缓存**，反复重读落盘，磁盘只是如实服务。结论：磁盘指标异常 ≠ 应用受害，必须回链到应用路径。

## 9.2.2 perf

```bash
perf record -e block:block_rq_issue -e block:block_rq_insert \
           -e block:block_rq_complete -a sleep 1
perf script       # 后处理拼延迟
```

三个跟踪点全量采集到**用户态**再分析；对比 BPF（biolatency）在**内核态聚合**，perf 方式在高 IOPS 系统上数据搬运开销大——与第 8 章 perf record 文件系统写的自反馈教训同理。

## 9.2.3 blktrace / btrace

blktrace 是块层事件的老牌采集器，操作识别符：

| 符号 | 事件 |
|------|------|
| Q | 请求进入块层（queued） |
| G | 生成 request（get request） |
| P | plug（蓄流合并窗口开启） |
| M | 合并（merge）回 request |
| D | 发布到驱动（issued to driver） |
| C | 设备完成（completed） |

`Q→D` 是等待时长、`D→C` 是服务时长——手工版"请求时长拆解"。可配 `seekwatcher` 做时间轴可视化。缺点：**繁忙系统上开销可观**、日志海量、都要落用户态处理。

## 9.2.4 SCSI 日志

```bash
sysctl dev.scsi.logging_level=...   # 位域控制各级别
scsi_logging_level -s ...           # sg3-utils 更友好
dmesg -w                            # 看输出
```

能拿到驱动层错误与命令级细节，但输出**缺请求标识符**，难以配对起止算延迟——这正是 BPF（scsilatency/scsiresult）的用武之地。

## HFT 关联

- 例行巡检用 `iostat -dxz 1` 够用；**排查延迟尖峰**时直接跳过 blktrace 用 biolatency/biosnoop，省去日志搬运和配对
- `await` 突增但吞吐正常 → 怀疑个别慢 I/O（分布长尾），iostat 看不见，直接上 biolatency

## 常见陷阱

- 忘了 `-z`：空闲设备刷屏，淹没真有问题的那块盘
- 在 NVMe 上用 %util 判饱和（重复 9.1 的坑）
- blktrace 开在生产高负载盘上不设时长——日志写爆磁盘

<details>
<summary>自测题</summary>

1. blktrace 的 Q、D、C 三个符号分别对应请求时长中的哪两段？
   <details><summary>答案</summary>Q→D 是等待时长（进块层到发布给驱动，含排队/合并/调度）；D→C 是服务时长（驱动到设备完成）。两段相加 = 完整请求时长——这就是 biolatency -Q（排队）与默认模式（总时长）各自测的东西的 blktrace 手工版。</details>

2. 为什么书中说 perf 三跟踪点方式开销大？
   <details><summary>答案</summary>perf record 把三个跟踪点的**全量事件**搬到用户态（perf.data），高 IOPS 系统上搬运本身就成了负载，还要事后 perf script 后处理拼延迟；BPF 在内核态 map 里完成配对与聚合，用户态只拿摘要。与第 8 章 perf record 自反馈循环是同一个教训的两面。</details>

3. SCSI 日志为什么算不了延迟？
   <details><summary>答案</summary>日志输出缺请求标识符——起止两条日志无法配对，没有配对就没有差值；这正是 scsilatency/scsiresult（BPF 用命令结构体指针做键）填的坑。</details>
</details>
