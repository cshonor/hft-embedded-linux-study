# 8.3 BPF 工具：VFS 层统计与文件生命周期（8.3.7–8.3.11）

> 底本：《BPF之巅》第 8 章 文件系统，8.3 节中段（印刷 p313–322）

| 工具 | 来源 | 一句话 |
|---|---|---|
| filelife | BCC/BT | 短命文件生死记录（创建→删除的 AGE） |
| vfsstat | BCC/BT | 每秒汇总 5 类常见 VFS 操作 |
| vfscount | BCC/BT | 统计全部 50+ 个 vfs_* 函数 |
| vfssize | BT | 按 (进程,文件) 直方图统计 VFS 读写尺寸 |
| fsrwstat | BT | 按文件系统类型拆分 VFS 读写（ext4 vs sockfs...） |

**共同注意**：VFS 层函数高频（生产可达每秒百万次），这组工具开销 1%~3% 量级，只适合临时调查，不适合 7×24 监控（监控要求 <0.1%）。

## filelife

```
TIME      PID   COMM  AGE(s)  FILE
17:04:51  3632  gcc   0.00    cc9JENsb.s      ← 编译期短命文件
17:04:51  3656  rm    0.00    version.h.tmp   ← 不到一秒即删
```

- 探针：kprobe `vfs_create` + `vfs_unlink`（新版内核 vfs_create 可能消失，退路是 `security_inode_create` LSM 钩子；两处都发时后者覆盖时间戳，对 AGE 计算影响不大）。
- 诞生时间戳以 **dentry 指针为键**存（全局唯一 ID），unlink 时算差并从 dentry 取文件名。
- 用途：抓"应用程序在不必要地使用临时文件"的优化点——每次创建/删除都是元数据 I/O。
- 选项：`-P PID`。

## vfsstat —— 顶层工作负载画像

```
TIME       READ/s   WRITE/s  CREATE/s  OPEN/s  FSYNC/s
02:41:24:  947879   34387    57883     1715013 10547
```

36-CPU Hadoop 生产实例：读超 100 万次/秒；**open 超 500 万次/秒**才是可疑信号（open 要路径查找+FD 分配+可能的 inode 创建，是慢操作）→ 用 opensnoop 下钻减少打开次数。

- 探针：kprobe vfs_read/vfs_write/vfs_fsync/vfs_open/vfs_create，interval:s:1 汇总打印（vmstat 风格，`vfsstat [interval [count]]`）。
- 局限：VFS 含网络/管道/proc，需 vfssize、fsrwstat 进一步拆分。

## vfscount —— 全函数普查

统计所有 vfs_* 函数（50+），funccount 的特化：

```bash
funccount 'vfs*'          # BCC 等价
bpftrace -e 'kprobe:vfs* { @[func] = count(); }'
```

实例结论：10 秒内 vfs_read 712610 次居首，vfs_fallocate 仅 1 次——快速识别负载主体。同样受"VFS 混网络"限制。

## vfssize —— I/O 尺寸直方图（按进程+文件）

```
@[grpc-default-wo,TCP]: [4,8] 1011 | [8,16] 12062 | ...   # 网络双峰
@[EVCACHE..,FIFO]: [1] 6376                               # 管道 1 字节读
@[tomcat-exec-393,tomcat_access.log]: [8K,16K] 31         # 唯一的真文件
```

48-CPU API 服务器实例的教训：**grpc 进程做了 266897 次 1 字节读**——典型 I/O 尺寸优化点（增大缓冲）。

实现：kprobe/kretprobe 四函数（vfs_read/readv/write/writev），入口存 `struct file*`，返回值即尺寸 `hist(retval)`。网络协议名恰好存在文件名位置（proto 结构体，见第 10 章）；FIFO 无名，固定输出 "FIFO"。

## fsrwstat —— 按文件系统类型拆分

```
@[ext4,vfs_write]:104268
@[sockfs,vfs_write]:1
@[pipefs,vfs_read]:160
```

解决"VFS 统计混入网络"问题：经 `file->f_inode->i_sb->s_type->name` 读文件系统类型字符串（sockfs/pipefs/devtmpfs/ext4...），键为 (fs类型, 函数名)。kprobe 不稳定接口，跨内核版本可能要改。

## HFT 关联

- vfsstat 的 OPEN/s 列是热路径文件句柄卫生检查：理想的交易进程启动后不再高频 open。
- vfssize 的 1 字节读检测同样适用于行情解析代码（读 syscall 次数 vs 每次搬运量的权衡）。
- filelife 抓配置热重载、临时落盘等隐式元数据 I/O。

## 常见陷阱

- vfsstat/vfscount 把 socket/pipe 算进 READ——先 fsrwstat 拆分再下"磁盘忙"结论。
- 这组工具在 VFS 百万次/秒的系统上开销 1–3%，调查用完即撤。

<details>
<summary>自测</summary>

1. vfsstat 实例中为什么 OPEN/s 比 READ/s 更值得警惕？
2. fsrwstat 是通过哪条指针链取到文件系统类型名的？
3. filelife 用什么作为文件的唯一 ID？为什么不用文件名？
</details>
