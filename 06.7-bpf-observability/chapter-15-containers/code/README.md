# 02-ch15 code · 容器域 bpftrace 程序

> 《BPF Performance Tools》第 15 章示例落盘。
> 笔记中 overlayfs.bt 完整版（§5 块5 引用的外部脚本）未在笔记给出全文，此处保留骨架说明。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [pidns-switch.bt](./pidns-switch.bt) | §4 块3 | PID 命名空间切换计数：CPU 在容器/宿主机间穿越的调度画像（nodename=pod 名） | 语义解析 ✓（实跑需 root） |
| [blkthrot.bt](./blkthrot.bt) | §5 块4 | blk-cgroup 节流按 css.id 统计（容器磁盘慢先查自己 cgroup → 06.6/ch11） | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [container-oneliners.sh](./scripts/container-oneliners.sh) | §2/5/6 | nsproxy 命名空间链路 / cgroup_id 采样 / cgroupid() 过滤指定容器 / overlayfs 骨架 | `bash -n` ✓ |

## 要点

- 内核认容器靠 `task->nsproxy`：pid_ns 编号区分"哪个容器"，uts_ns nodename 给名字
- `cgroupid("/sys/fs/cgroup/...")`：把 cgroup 路径变成可比较的 id（bpftrace 容器过滤的标准写法）
