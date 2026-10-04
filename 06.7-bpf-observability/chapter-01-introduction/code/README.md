# 02-ch01 code · 初识 BCC/bpftrace

> 《BPF Performance Tools》第 1 章示例落盘。
> 笔记中的预期输出展示块（块2/3/4 的输出部分）保留在笔记内。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [trace-open.py](./trace-open.py) | §9 块5 | BCC `TRACEPOINT_PROBE` 追踪 openat——与 bpftrace 一行形成"完整程序 vs 单行"对照 | `py_compile` ✓；C 编译需 root（map 创建阶段才报权限 = C 已过） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [probe-explore.sh](./scripts/probe-explore.sh) | §7 块1、§8 块2-4 | 探针勘探三件套（bpftrace -l / format 字段 / USDT）+ 书上三组对照实验 | `bash -n` ✓；①②段实跑 ✓（④需 root） |

## 要点

- `bpftrace -e` 单行适合勘探；BCC 完整程序适合复杂状态机（entry/exit 配对、多 map）
- `fprintf` 是书上旧版写法，bpftrace ≥0.17 用 `printf`
- glibc 的 `open()` 内部走 `openat`——所以挂 `sys_enter_open*` 时 openat 计数远多于 open
