# 02-ch08 code · 文件系统域 bpftrace 程序

> 《BPF Performance Tools》第 8 章示例落盘。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [vfs-read-by-file.bt](./vfs-read-by-file.bt) | §3 块4 | fd → 文件名反查链（task_struct→fdtable→dentry）统计 read | 语义解析 ✓（实跑需 root） |
| [read-by-fs.bt](./read-by-fs.bt) | §10 块2 | 按文件系统类型统计 vfs_read（ext4/tmpfs/overlay 分布） | 语义解析 ✓ |
| [fd-leak-watch.bt](./fd-leak-watch.bt) | §10 块3 | fd 泄漏看门狗：alloc_fd 记账 / close_fd 销账 / 10s 悬账打印 | 语义解析 ✓ |
| [creat-unlink-trace.bt](./creat-unlink-trace.bt) | §10 块1 | creat/unlink 追踪 + 文件存活时长（filelife 简化版，附局限说明） | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [fs-oneliners.sh](./scripts/fs-oneliners.sh) | §4 块5、§9 块6/7 | VFS 统计 / BCC 单行七条（trace/argdist/funccount/stackcount）/ bpftrace 单行五条 | `bash -n` ✓ |

## 本章核心：fd 反查文件名

read(2) 只有 fd 号；`task->files->fdt->fd[fd]` → `struct file*` → `f_path.dentry->d_name.name`
——内核里"从整数句柄找回路径"的通用链路，socket/fd 场景同理。
