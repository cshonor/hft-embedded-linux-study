# 8.1 背景知识

> 底本：《BPF之巅》第 8 章 文件系统，8.1 节（印刷 p291–300）

## I/O 软件栈（图 8-1）

```
应用（缓冲 I/O / 直接 I/O / 内存映射 mmap / 裸 I/O）
  ↓ POSIX 库
系统调用层（read/write/open/fsync...）
  ↓
VFS 虚拟文件系统 —— 【逻辑 I/O】从这里测量
  ↓
文件系统（ext4 / XFS / btrfs / NFS...）
  ↓
卷管理器（LVM / md raid）
  ↓
块设备驱动 —— 【物理 I/O】在这里测量
  ↓
存储设备
```

| 概念 | 定义 | 测量层 |
|---|---|---|
| 逻辑 I/O | VFS 层看到的读写请求 | filetop / vfssize |
| 物理 I/O | 真正下发到块设备的请求 | 第 9 章工具（biolatency 等） |
| 裸 I/O | 绕过文件系统直接访问设备 | 第 9 章 |
| 直接 I/O | 绕过页缓存（O_DIRECT） | writevsync 可识别 |

**关键点**：逻辑 I/O ≠ 物理 I/O。页缓存命中时逻辑 I/O 有百万次而物理 I/O 为零；预读取/合并又会改变两者的数量与尺寸关系。文件系统层分析（本章）往往比单纯磁盘分析（第 9 章）对应用更有解释力——fileslower 测到的延迟一定阻塞应用，而磁盘层高延迟未必影响应用。

## 文件系统缓存（图 8-2）

| 缓存 | 存什么 | 本章工具 |
|---|---|---|
| 页缓存（最大） | 文件内容页，含**脏页**（待写回） | cachestat / writeback / readahead |
| inode 缓存 | 文件元数据 | icstat |
| dcache 目录缓存 | 路径名→inode 映射，路径查找加速 | dcstat / dcsnoop |
| 缓冲缓存 | 块设备的缓冲头（页缓存的块页） | bufgrow |

## 两个机制

- **预读取（readahead）**：内核检测到顺序读时提前读入后续页。仅顺序读受益；随机读 workload 下预读取全是浪费（readahead 工具可量化；SSD 上预读取收益下降甚至为负——Netflix 生产案例，详见 9 章 biosnoop）。
- **写回（write-back）**：写操作只进页缓存即返回（写回模式），脏页由内核周期性/后台刷盘。对比**写穿透**（O_SYNC，每次写等存储完成）——writesync 工具用来揪出误用同步写的进程。

## BPF 能力（表 8-1 事件源）

| 层 | 事件源 | 说明 |
|---|---|---|
| 应用/库 | uprobes、USDT | libc 的文件操作 |
| 系统调用 | tracepoint:syscalls | 稳定、开销低 |
| VFS | kprobes（vfs_read 等） | 频率高，注意开销 |
| 文件系统 | ext4 跟踪点 >100 个；XFS 约 500 个 | 稳定好用于深挖 |
| 写回 | tracepoint:writeback | writeback_start/written |
| 块层 | tracepoint:block | 第 9 章 |

注意：VFS 层操作同时覆盖**网络 socket、/proc、管道**（一切皆文件），vfsstat 看到的高读频率可能来自网络——用 fsrwstat 按文件系统类型拆分。物理 I/O 通常 <1000 IOPS，块层跟踪开销可忽略。

## 十步分析策略（8.1.3）

1. `df -h` / `mount` 检查容量与挂载选项（容量 >90% 会因空余块碎片化而性能下降；noatime 可省 atime 更新）
2. 已知负载下测延迟（fio 生成基准）
3. `opensnoop` 看打开了什么
4. `filelife` 看短命文件
5. `ext4slower`（或 xfsslower）找慢操作
6. `ext4dist` 看延迟分布（双峰=缓存命中 vs 磁盘）
7. `cachestat` 看页缓存命中率
8. `vfsstat` 对比逻辑 vs 物理 I/O 量

## HFT 关联

- 交易系统的行情落盘、订单日志（append-only）顺序写模式：写回模式 + 预读取是默认优化，但要警惕 fsync 风暴（syncsnoop 监控）。
- 回测/历史数据加载是典型大文件顺序读，页缓存命中率直接决定回测吞吐（cachestat + readahead）。

## 常见陷阱

- 在 VFS 层统计 I/O 却忘了 socket 也走 VFS，误判"磁盘忙"。
- 用容量百分比判断性能：90% 以下通常没事，>90% 空闲块分散导致新写入碎片化。

<details>
<summary>自测</summary>

1. 逻辑 I/O 和物理 I/O 分别在哪一层测量？为什么 fileslower 的高延迟比磁盘层高延迟更能证明应用受影响？
2. 页缓存里的脏页是什么？谁负责、何时刷回磁盘？
3. 为什么 vfsstat 看到读 IOPS 百万次不等于磁盘忙？
</details>
