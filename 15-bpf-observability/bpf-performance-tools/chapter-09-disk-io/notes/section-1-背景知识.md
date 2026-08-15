# 1. 背景知识（9.1）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.1 节（印刷 p361–374）

## 9.1.1 磁盘系统基础：块 I/O 软件栈

图 9-1 展示了存储 I/O 从应用到磁盘要穿越的各层：

```
应用（read/write/fsync）
   ↓
文件系统 / 裸块设备（直接打开 /dev/sdX）
   ↓
块设备接口
   ↓
卷管理器（md/RAID、LVM）
   ↓
设备映射器（device-mapper）
   ↓
块 I/O 层（bio / request）
   ↓
I/O 调度器（传统：Noop/Deadline/CFQ；多队列：None/BFQ/mq-deadline/Kyber）
   ↓
HBA 驱动（SCSI/NVMe/…）
   ↓
磁盘设备
```

两个核心结构体：

| 结构体 | 含义 | 携带信息 |
|--------|------|----------|
| `bio` | 块 I/O 操作的内核描述 | 目标设备、扇区、字节数、读/写方向 |
| `request` | 调度器加工后的 I/O（可合并多个 bio） | 同上 + 状态供延迟统计 |

### rwbs：I/O 类型标记字符串

`blk_fill_rwbs()` 填充的字符串，是块层所有工具的"T 列"来源，**可组合**（如 `WM` = 写+元数据）：

| 字符 | 含义 |
|------|------|
| R | 读 |
| W | 写 |
| M | 元数据（文件系统内部操作） |
| S | 同步（写完才算完成） |
| A | 预读 |
| F | 强制写屏障/flush |
| D | 丢弃（TRIM） |
| E | 擦除 |

### I/O 调度器

- **传统调度器**（Noop / Deadline / CFQ）：全局请求锁是瓶颈，**Linux 5.0 已删除**
- **多队列调度器**（multi-queue）：None / BFQ / mq-deadline / Kyber，按硬件队列分发，锁竞争小

### 图 9-2 术语（务必分清）

```
应用发布 I/O
   |<------------ 请求时长 (request) ------------>|
   |<-- 等待时长 -->|<---------- 服务时长 -------->|
   |   (OS 调度器+   |  (发布到设备 → 设备完成，    |
   |    分发队列)    |   含设备自带队列排队)         |
```

- **请求时长**：最重要。同步 I/O 中应用真正等待的时间
- **等待时长**：在 OS 调度器/分发队列中的排队时间
- **服务时长**：发布到设备至设备报告完成
- 磁盘**使用率只测繁忙度**，不是性能指标；多设备虚拟盘（RAID/md）上 `%util` 有误导性

## 9.1.2 BPF 能力（表 9-1）

| 层 | 事件源 |
|----|--------|
| 块 I/O 层 | `block:*` 跟踪点（`block_rq_issue`、`block_rq_complete`、`block_rq_insert`…）、kprobe（`blk_account_io_start/done`） |
| I/O 调度器 | kprobe `elv_add_request` 等 |
| SCSI | `scsi:*` 跟踪点（`scsi_dispatch_cmd_start/done`） |
| NVMe | kprobe `nvme_setup_cmd`/`nvme_complete_rq`（nvme 跟踪点较新才加入） |

`block:block_rq_issue` 跟踪点参数：`dev`、`sector`、`nr_sector`、`bytes`、`rwbs`、`comm`、`cmd`——够写绝大多数块层工具。

## 9.1.3 分析策略（五步）

1. **先从文件系统层入手**（第 8 章）：问题常在页缓存/应用 I/O 模式，而非磁盘
2. `iostat` 看基本指标：IOPS、吞吐、await
3. `biolatency` 看延迟**分布**——多峰即线索（不同介质/排队/坏设备）
4. `biosnoop` 逐事件看**模式**：顺序还是随机、尺寸、是否排队
5. 需要归因时再上 `biostacks`/`biotop`/调度器/驱动层工具

> ⚠️ 告诫：**异步 I/O（写回、预读）不受磁盘延迟直接影响应用**——看到块层延迟高，先确认是不是应用同步等待的路径，否则可能白忙。

## HFT 关联

- 交易系统的订单落盘（WAL/日志）是**同步写**路径：请求时长 = 应用等待时长，`FS`/`WS` 标记的延迟直接进撮合/回报延迟
- NVMe 多队列下调度器选 None/mq-deadline 即可，重点是 `biolatency -F` 按类型分桶排除邻居干扰

## 常见陷阱

- 把 `%util` 100% 当"磁盘过载"——多队列/NVMe 设备可并行，繁忙≠饱和
- 把平均 await 当真相——双峰分布（如 128–2047µs + 4–32ms）平均后两边都看不见
- Linux 5.0+ 内核上照抄老工具的 `blk_start_request`/`scsi_done` kprobe——这些函数已删

<details>
<summary>自测题</summary>

1. rwbs 为 `WSM` 的 I/O 是什么操作？
2. 请求时长、等待时长、服务时长三者关系？
3. 为什么说 %util 在 NVMe 上有误导性？
4. 为什么策略第一步是文件系统层而不是块层？

</details>
