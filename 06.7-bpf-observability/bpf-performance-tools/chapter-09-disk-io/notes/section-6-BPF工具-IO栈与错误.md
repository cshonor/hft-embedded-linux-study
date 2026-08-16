# 6. BPF 工具：I/O 栈与错误（biostacks / bioerr / mdflush）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.3.7–9.3.9 节（印刷 p399–402）

## biostacks（BCC / BT）

给每个块 I/O 记录**内核栈**：`blk_account_io_start` 时存 kstack + 时间戳，`blk_start_request` / `blk_mq_start_request`（发布到设备）时打印。

书中两例展示同一块层事件背后的完全不同来源：

| 栈来源 | 含义 |
|--------|------|
| 缺页换页栈 | 读盘是**页缓存 miss → 换页** 引起，应用并无 read() 调用 |
| `newfstatat` → 预读栈 | 是 **stat 触发的 inode 预读**（呼应第 8 章 Netflix 微服务 stat 风暴案例） |

另有 ZFS 后台校验（scrub）进程案例：块层看到的读，源头是文件系统的后台任务而非任何应用。

> biostacks 是"块层事件 ↔ 内核发起路径"的翻译器——第 8 章工具看到的是进程，这里能看到更底层的因果链。

## bioerr（BCC / BT）

统计块 I/O 错误：`block_rq_complete` 过滤 `args->error != 0`，按 [设备, 错误码] 计数。

书中案例：设备 0,0 每 2 秒一次 EIO(-5)——追查发现是 **USB 存储检测的 `scsi_test_unit_ready`**，属正常探测而非数据错误。错误码反查链：`blk_status_to_errno` → `BLK_STS_IOERR = 10`。

方法论：**先量化错误频率与来源，再判性质**——不是所有非零 error 都是故障。

## mdflush（BCC / BT）

跟踪 md（软 RAID）设备的 flush 请求：kprobe `md_flush_request`。

书中案例：filebeat（日志采集）**每 5 秒**对 md0 发 flush——把周期性延迟尖峰和具体应用行为对上号。flush（rwbs=F）会把设备写缓存刷穿，代价高，周期性来源值得审计（呼应第 8 章 syncsnoop 的 fdatasync 时间轴分析）。

## HFT 关联

- 交易系统出现周期性（如每 5s/60s）盘延迟尖峰：biostacks/mdflush 配合时间轴找"定时器型"发起者（监控 agent、日志 rotate、备份）
- bioerr 常态化运行可提前发现盘降级（错误率爬升先于彻底损坏）

## 常见陷阱

- bioerr 看到 EIO 就报故障——先看设备号与 comm，可能是探测类正常错误
- biostacks 的栈是**内核栈**；要看用户态调用路径得配第 8 章 fileslower/filetop（进程还在时）

<details>
<summary>自测题</summary>

1. biostacks 用哪两个事件配对？为什么完成点选 blk_start_request 而非 rq_complete？
2. 书中 EIO 每 2 秒一次的真实原因是什么？
3. mdflush 解决什么"尖峰归因"问题？

</details>
