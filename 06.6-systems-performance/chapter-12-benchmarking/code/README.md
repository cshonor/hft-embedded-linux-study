# ch12 code · 基准测试脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [fio-latency-vs-throughput.sh](./scripts/fio-latency-vs-throughput.sh) | 12.2 块3 | iodepth=1（真实延迟）vs iodepth=32（排队吞吐）对照基准 + Little's Law 校验 | `bash -n` ✓；fio 本机**未装**（`sudo apt install fio`） |

## 运行前提

- `fio`（未装）；默认 `--rw=randread --readonly` 不破坏数据，
  测写路径时自己改 `--rw=randwrite`（会写裸设备，确认设备无数据！）
