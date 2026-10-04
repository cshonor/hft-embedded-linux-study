# 02-ch11 code · 安全域 bpftrace 程序

> 《BPF Performance Tools》第 11 章示例落盘。
> 笔记中 capable() 内核源码（§2 块3）为引用，不落盘。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [rename-exchange-watch.bt](./rename-exchange-watch.bt) | §1 块1/2 | RENAME_EXCHANGE 原子交换监控 + `--unsafe` signal(9) 管控版（观测→主动防御） | 语义解析 ✓（实跑需 root） |
| [session-forensics.bt](./session-forensics.bt) | §4 块5 | 会话取证：sched_process_fork 记后代 → 后代 write 内容全打印（**授权系统专用**） | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [security-oneliners.sh](./scripts/security-oneliners.sh) | §2 块4、§8 块6 | BPF 安全 sysctl 读数 / LSM 钩子审计 / PAM 追踪 / 模块加载追踪 | `bash -n` ✓；sysctl 段实跑 ✓ |

## 要点

- `signal()` 是 `--unsafe` 功能：bpftrace 从观测升级为处置（杀进程）——与 ch09-01 的 LSM 拒绝形成"软/硬"两级管控
- 本机 `kernel.unprivileged_bpf_disabled=1`（默认安全姿态），生产建议 `bpf_jit_harden=1`
