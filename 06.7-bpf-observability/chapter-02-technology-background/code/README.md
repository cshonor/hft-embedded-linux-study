# 02-ch02 code · 技术背景示例

> 《BPF Performance Tools》第 2 章示例落盘。
> 笔记中 TRACE_EVENT 宏定义（§9 块10）、folly 版 USDT 写法（§10 块1/3）为源码引用，
> 落盘版改用更通用的 `sys/sdt.h`。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [usdt-tick.c](./usdt-tick.c) | §10 块1/3/4 补全 | USDT 最小 demo：`DTRACE_PROBE1(tick, loop, i)` + readelf 验证 + bpftrace 挂载 | ⚠ 本机缺 `systemtap-sdt-dev`（需 sudo 装），未编译；readelf 验证步骤见文件注释 |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [bpftool-cheatsheet.sh](./scripts/bpftool-cheatsheet.sh) | §3 块5 | bpftool：prog show/dump xlated(+linum)/dump jited/map show/btf dump | `bash -n` ✓；prog show 段实跑 ✓（本机已加载程序可见） |
| [probe-types.sh](./scripts/probe-types.sh) | §7/8/9 块6-13 | 四种挂点对比：kprobe/uprobe/tracepoint/USDT + tracepoint_exists 兼容写法 + 选型口诀 | `bash -n` ✓ |
