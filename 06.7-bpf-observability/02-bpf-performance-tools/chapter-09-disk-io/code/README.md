# 02-ch09 code · 块 I/O 域 bpftrace 程序

> 《BPF Performance Tools》第 9 章示例落盘。
> iostat/perf/scsi_logging 传统工具（§2 块1-3）保留在笔记内；
> biolatency/biosnoop 等成品已在 06.6/ch09 落盘。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [blk-plug-latency.bt](./blk-plug-latency.bt) | §8 块5 | 块层 plug 蓄流时间直方图——配对键用 arg0（对象指针）而非 tid（跨线程配对的要点） | 语义解析 ✓（实跑需 root） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [disk-oneliners.sh](./scripts/disk-oneliners.sh) | §8 块4-6 | BCC 八条 + bpftrace 九条（尺寸直方图/rwbs 标记/错误逐次/SCSI opcode/CPU 分布） | `bash -n` ✓ |

## 本章核心：配对键的选择

`blk_start_plug` 和 `blk_flush_plug_list` 可能在**不同线程**执行——
用 `@ts[tid]` 配对会丢，必须用 `@ts[arg0]`（plug 对象地址）作为关联键。
判断标准：配对的两个事件是否保证同一线程上下文。
