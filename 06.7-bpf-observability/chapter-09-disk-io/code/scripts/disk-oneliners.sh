#!/bin/bash
# disk-oneliners.sh — 块 I/O 域命令精选（02-ch09 §8 块4-6）
# 全部需要 root 执行
set -u

cat <<'EOF'
===== BCC 单行 =====
funccount-bpfcc 't:block:*'                                     # 块 tracepoint 覆盖
argdist-bpfcc -H 't:block:block_rq_issue():u32:args->bytes'     # I/O 尺寸直方图
stackcount-bpfcc -ut 't:block:block_rq_issue'                   # 用户态调用栈
argdist-bpfcc -c 't:block:block_rq_issue():char*:args->rwbs'    # rwbs 类型标记
trace-bpfcc 't:block:block_rq_complete (args->error) "dev %d type %s error %d", args->dev, args->rwbs, args->error'
argdist-bpfcc -c 't:scsi:scsi_dispatch_cmd_start():u32:args->opcode'   # SCSI opcode
argdist-bpfcc -c 't:scsi:scsi_dispatch_cmd_done():u32:args->result'    # SCSI 结果码
funccount-bpfcc 'nvme*'

===== bpftrace 单行 =====
bpftrace -e 'tracepoint:block:* { @[probe] = count(); }'
bpftrace -e 't:block:block_rq_issue { @bytes = hist(args->bytes); }'
bpftrace -e 't:block:block_rq_issue { @[ustack] = count(); }'
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = count(); }'
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = sum(args->bytes); }'
bpftrace -e 't:block:block_rq_complete /args->error/ { printf("dev %d type %s error %d\n", args->dev, args->rwbs, args->error); }'
bpftrace -e 't:scsi:scsi_dispatch_cmd_start { @opcode[args->opcode] = count(); }'
bpftrace -e 't:scsi:scsi_dispatch_cmd_done { @result[args->result] = count(); }'
bpftrace -e 't:block:block_rq_issue { @[cpu] = count(); }'      # 请求的 CPU 分布

===== plug 蓄流计时（已落成完整脚本） =====
sudo bpftrace ../blk-plug-latency.bt

===== 配套（06.6/ch09 已落盘） =====
biolatency-bpfcc / biosnoop-bpfcc / 慢 I/O bpftrace —— 见 06.6-systems-performance/chapter-09-disks/code/
EOF
