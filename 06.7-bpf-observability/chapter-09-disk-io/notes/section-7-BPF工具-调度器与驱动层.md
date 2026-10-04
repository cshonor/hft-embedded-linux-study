# 7. BPF 工具：调度器与驱动层（iosched / scsilatency / scsiresult / nvmelatency）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.3.10–9.3.13 节（印刷 p402–406）

## iosched（BCC / BT）

I/O 在**调度器里排队多久**：`elv_add_request`（入队）→ `blk_start_request` / `blk_mq_start_request`（出队发布），输出含调度器名（从 request 结构体取）。

书中案例：CFQ 调度器排队 **8–64ms**——排队时长（9.1 图 9-2 的等待时长）单独可见，与设备快慢解耦。对应 biosnoop QUE 列的聚合视角。

## scsilatency（BCC / BT）

SCSI 命令延迟：`scsi_init_command`（起）→ `scsi_done` / `scsi_mq_done`（止），按 opcode 分直方图。opcode 映射表（节选）：

| opcode | 命令 |
|--------|------|
| 0x00 | TEST UNIT READY |
| 0x28 | READ 10 |
| 0x2a | WRITE 10 |

> ⚠️ Linux 5.0 移除了 `scsi_done`（传统单队列路径），新内核用 `scsi_mq_done`。

## scsiresult（BCC / BT）

SCSI 结果码分布：跟踪点 `scsi:scsi_dispatch_cmd_done`，解码 `args->result` 的 4 字节：

```
driver_byte << 24 | host_byte << 16 | msg_byte << 8 | status_byte
```

- `result>>16 & 0xff` = host byte（查 `DID_*` 表）
- `result & 0xff` = status byte（查 `SAM_STAT_*` 表）
- 双映射表在内核 `include/trace/events/scsi.h`；driver/msg 字节本工具未输出，可作扩展键
- 跟踪点还带 host_no/channel/id/lun/opcode/cmd_len 等，均可做键

## nvmelatency（BT）

按磁盘 + NVMe 命令码统计命令延迟：kprobe `nvme_setup_cmd`（起，存 `@start[arg1]`、`@cmd[arg1]`）→ `nvme_complete_rq`（止，从 `nvme_command.common.opcode` 解码）。命令表：0x00 flush / 0x01 write / 0x02 read / 0x04 write uncor / 0x08 write zeroes…

开发过程本身就是教材（作者 2019-03-21 完成）：

1. 系统**没有 nvme 跟踪点** → 先 `funccount 'nvme*'`（184 探针）统计各函数频次
2. 读源码确定延迟边界 = `nvme_setup_cmd()` → `nvme_complete_rq()`
3. **去看新版内核 nvme 跟踪点的实现源码**，学到正确读取 opcode 的方法——即使目标系统没有跟踪点，跟踪点实现也是"怎么读这个结构体"的权威示例
4. 无 `rq_disk` 的请求是**管理命令**（admin command），单独计数

> 这是"没有现成跟踪点时如何用 kprobe + 驱动源码自建工具"的完整方法论，值得原样复用到任何驱动层观测。

## HFT 关联

- NVMe 交易盘：nvmelatency 把 flush 与 read/write 分开统计——WAL 的 flush 延迟（写屏障穿缓存）才是落盘确认的真正耗时
- iosched 验证 mq-deadline/Kyber 的排队参数调整是否生效

## 常见陷阱

- Linux 5.0+ 上 kprobe `scsi_done`/`blk_start_request` 不存在——工具要写 `scsi_mq_done`/`blk_mq_start_request` 分支
- nvmelatency 忘处理管理命令 → admin 请求无 rq_disk，直接解引用会出错

<details>
<summary>自测题</summary>

1. scsiresult 的 result 四字节各是什么？取 host byte 的位运算？
   <details><summary>答案</summary>`driver_byte<<24 | host_byte<<16 | msg_byte<<8 | status_byte`；host byte = `result>>16 & 0xff`（查 DID_* 表），status byte = `result & 0xff`（查 SAM_STAT_* 表）。映射表在内核 `include/trace/events/scsi.h`。</details>

2. nvmelatency 的作者在没有跟踪点的系统上如何确定 kprobe 边界和 opcode 读法？
   <details><summary>答案</summary>四步：① funccount 'nvme*' 摸底各函数频次（184 个探针里找热点）；② 读驱动源码确定延迟边界（nvme_setup_cmd→nvme_complete_rq）；③ 去看**新版内核** nvme 跟踪点的实现源码——跟踪点实现是"怎么读这个结构体"的权威示例；④ 识别管理命令（无 rq_disk）单独计数防解引用出错。这套流程可原样复用到任何驱动层观测。</details>

3. iosched 的入队/出队各用哪个探针？
   <details><summary>答案</summary>入队 `elv_add_request`，出队 `blk_start_request`/`blk_mq_start_request`（发布到设备）——输出的是调度器内排队时长，与设备服务时长解耦。</details>
</details>
