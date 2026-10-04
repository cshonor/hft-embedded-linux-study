# 8.6 小结

> 底本：《BPF之巅》第 8 章 文件系统，8.6 节（印刷 p360）

## 本章工具全景（按 I/O 栈层次）

| 层 | 工具 |
|---|---|
| 系统调用 | opensnoop、statsnoop、syncsnoop、scread、mmapfiles |
| VFS | filelife、vfsstat、vfscount、vfssize、fsrwstat、fileslower、filetop、writesync、filetype、fmapfault、mountsnoop |
| 文件系统 | xfsslower/xfsdist、ext4dist（+btrfs/zfs/nfs 族） |
| 缓存 | cachestat、writeback、dcstat、dcsnoop、icstat、bufgrow、readahead |

## 一句话总结（原书）

BPF 工具覆盖了系统调用、VFS、文件系统函数与跟踪点四层跟踪，写回与预读取两个机制，以及页缓存/dcache/inode 缓存/缓冲缓存四种缓存；延迟直方图工具（xfsdist/ext4dist 等）用于识别多峰分布与特殊情况，最终服务于解决应用程序的性能问题。

### 四种缓存 × 各自的健康指标

| 缓存 | 工具 | 健康形态 | 异常形态 |
|------|------|---------|---------|
| 页缓存（文件数据） | cachestat | 活跃数据集 100% 命中 | HITRATIO 掉 → 冷读进延迟路径 |
| dcache（路径→inode） | dcstat | 99%+，SLOW 低 | miss 高 → 深目录/冷路径设计代价 |
| inode 缓存 | icstat | 稳态近 100% | 骤降 → 冷元数据扫描（find/遍历） |
| 缓冲缓存 | bufgrow | 稳态增长有限 | 持续增长 → 争内存（与 ch7 页缓存消费合并看） |

### 排障的层次纪律（本章可迁移的方法论）

```
应用说"文件慢" → 先问哪一层慢：
  syscalls 层（opensnoop/scread）   —— 调用方式蠢不蠢？（1 字节读/高频 stat）
  VFS 层（fileslower/filetop）      —— 哪个文件慢？
  FS 层（ext4dist/xfsdist）         —— 分布是多峰还是拖尾？
  缓存层（cachestat/dcstat）        —— 是不是该命中的没命中？
  （还慢 → 交给 ch9 块层/设备）
```
自上而下逐层收窄，每层一个工具一句话就能筛掉一层——这正是 BPF 按层观测的结构红利。

## 与相邻章衔接

- **上游 第 7 章（内存）**：页缓存本身就是内存消费者——cachestat 的 CACHED_MB、bufgrow 的缓冲增长都从文件系统侧解释了 free 输出；mmap 文件的缺页（fmapfault）是 7 章 faults 的子集。
- **下游 第 9 章（磁盘 I/O）**：本章 fileslower/xfsslower 测的是"应用视角的文件延迟"，第 9 章测"设备视角的块延迟"；readahead 遗留的 SSD 预读问题在 biosnoop 续讲；vfsstat(逻辑) × biolatency(物理) 对比是容量分析标配。
