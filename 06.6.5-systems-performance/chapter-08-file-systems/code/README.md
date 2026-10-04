# ch08 code · 文件系统示例与脚本

> 本章笔记中的程序类代码块补全/落盘；说明性伪代码块保留在笔记内。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [fadvise_demo.c](./fadvise_demo.c) | 8.1-8.3 块1 | `posix_fadvise` 两提示：SEQUENTIAL（加大预读）+ DONTNEED（防回放挤掉热缓存） | gcc 13.3 编译 ✓ 运行 ✓（读文件报 MB/s + 发 DONTNEED） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [fs-triage.sh](./scripts/fs-triage.sh) | 8.5 块2/3 | 排查五连：挂载选项(noatime)/free/sar -v/cachestat/ext4slower + drop_caches 警示 | `bash -n` ✓；cachestat/ext4slower-bpfcc 已装（需 root） |
| [fio-seqread.sh](./scripts/fio-seqread.sh) | 8.7-8.8 块4 | fio 顺序读基准（direct=1 + 分位数输出）+ 对照实验建议 | `bash -n` ✓；fio 本机**未装**（`sudo apt install fio`） |
