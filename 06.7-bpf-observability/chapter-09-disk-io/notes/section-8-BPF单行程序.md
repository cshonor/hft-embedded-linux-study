# 8. BPF 单行程序（9.4）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.4 节（印刷 p406–409）

## 9.4.1 BCC 单行

```bash
# 统计块 I/O 跟踪点调用
funccount 't:block:*'

# 直方图统计块 I/O 尺寸
argdist -H 't:block:block_rq_issue():u32:args->bytes'

# 块 I/O 请求的用户态调用栈
stackcount -ut 't:block:block_rq_issue'

# 统计块 I/O 类型标记（rwbs）
argdist -c 't:block:block_rq_issue():char*:args->rwbs'

# 按设备和 I/O 类型跟踪块 I/O 错误
trace 't:block:block_rq_complete (args->error)' \
      '"dev %d type %s error %d", args->dev, args->rwbs, args->error'

# 统计 SCSI opcode
argdist -c 't:scsi:scsi_dispatch_cmd_start():u32:args->opcode'

# 统计 SCSI 结果代码
argdist -c 't:scsi:scsi_dispatch_cmd_done():u32:args->result'

# 统计 nvme 驱动函数
funccount 'nvme*'
```

## 9.4.2 bpftrace 单行

```bash
# 统计块 I/O 跟踪点
bpftrace -e 'tracepoint:block:* { @[probe] = count(); }'

# 直方图统计块 I/O 尺寸
bpftrace -e 't:block:block_rq_issue { @bytes = hist(args->bytes); }'

# 块 I/O 请求的用户态调用栈
bpftrace -e 't:block:block_rq_issue { @[ustack] = count(); }'

# 统计块 I/O 类型标记
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = count(); }'

# 按 I/O 类型统计总字节数
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = sum(args->bytes); }'

# 按设备和 I/O 类型跟踪块 I/O 错误
bpftrace -e 't:block:block_rq_complete /args->error/ {
    printf("dev %d type %s error %d\n", args->dev, args->rwbs, args->error); }'

# 直方图统计块 I/O plug（蓄流）时间
bpftrace -e 'k:blk_start_plug { @ts[arg0] = nsecs; }
    k:blk_flush_plug_list /@ts[arg0]/ { @plug_ns = hist(nsecs - @ts[arg0]);
    delete(@ts[arg0]); }'

# 统计 SCSI opcode
bpftrace -e 't:scsi:scsi_dispatch_cmd_start { @opcode[args->opcode] = count(); }'

# 统计 SCSI 结果代码（含全部 4 字节）
bpftrace -e 't:scsi:scsi_dispatch_cmd_done { @result[args->result] = count(); }'

# 统计 blk_mq 请求的 CPU 分布
bpftrace -e 't:block:block_rq_issue { @[cpu] = count(); }'

# 统计 scsi / nvme 驱动函数
funccount 'scsi*'; funccount 'nvme*'
```

## 9.4.3 单行程序示例：rwbs 频率统计

```bash
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = count(); }'
```

实测输出（节选）：

```
@[F]:     12
@[WM]:    64
@[WS]:    86
@[RA]:  2128     ← 预读
@[R]:   3635     ← 读
@[W]:   4578     ← 写
```

一次性回答负载定性问题：

- 读请求 vs 预读请求比例？（R vs RA）
- 写 vs 同步写比例？（W vs WS）
- 把 `count()` 换成 `sum(args->bytes)` 即变成**按类型的字节量**分布——IOPS 视角与吞吐视角一键切换

## HFT 关联

- 开盘前快照：rwbs 频率 + 字节两条单行，10 秒看清盘上负载构成（同步写占比高 → 对延迟敏感）
- plug 时间直方图：多队列蓄流窗口会人为引入微秒~毫秒级延迟，低延迟场景审计 `none` 调度器时有用

## 常见陷阱

- `tracepoint:block:*` 通配会挂上所有 block 跟踪点（含 insert/merge/remap），读输出时注意区分
- rwbs 是字符串（`char*`），BCC argdist 用 `char*:args->rwbs`，别当整数取

<details>
<summary>自测题</summary>

1. 如何一条命令从"IOPS 按类型"切换为"字节量按类型"？
   <details><summary>答案</summary>把 `@[args->rwbs] = count()` 换成 `@[args->rwbs] = sum(args->bytes)`——键不变、聚合函数从计数换求和。count() 回答"次数"，sum() 回答"体积"：小 I/O 高频时两者画像完全不同（IOPS 视角吓人、吞吐视角无辜）。</details>

2. plug/unplug 蓄流的意义是什么？哪个单行测量它？
   <details><summary>答案</summary>plug 是块层的合并窗口：发起方先攒一批请求（plug）再一次性交给调度器，目的是把相邻小请求合并成大 I/O 提升顺序性——代价是人为引入微秒~毫秒级的蓄流等待。测量单行：`k:blk_start_plug { @ts[arg0]=nsecs; } k:blk_flush_plug_list /@ts[arg0]/ { @plug_ns=hist(nsecs-@ts[arg0]); delete(@ts[arg0]); }`（双探针计时模板，键是 task 结构体指针）。</details>
</details>
