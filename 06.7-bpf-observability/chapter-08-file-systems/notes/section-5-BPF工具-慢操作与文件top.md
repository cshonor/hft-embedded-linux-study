# 8.3 BPF 工具：慢操作、文件 Top 与写模式（8.3.12–8.3.15）

> 底本：《BPF之巅》第 8 章 文件系统，8.3 节中段（印刷 p322–331）

| 工具 | 来源 | 一句话 |
|---|---|---|
| fileslower | BCC/BT | 超阈值的**同步**读/写逐事件打印 |
| filetop | BCC | 文件版 top(1)，按字节排读写最热的文件 |
| writesync | BT | 揪出带 O_DSYNC 同步写标志的进程/文件 |
| filetype | BT | 按文件类型（REG/SOCK/FIFO...）拆分 VFS 读写 |

## fileslower —— 离应用最近的延迟证据 🔴

```
# fileslower          # 默认阈值 10ms
TIME(s) COMM  TID    D BYTES  LAT(ms)  FILE
0.142   java  111264 R 4096   25.53    part-00762-...
2.605   java  47785  W 64512  34.45    blk2191481297
```

**为什么它比磁盘层工具更能定罪**：跟踪的是**同步**读/写——进程完全阻塞在这上面等返回。fileslower 显示高延迟 ⇒ 应用大概率真的受害；反过来磁盘层测到的高延迟可能只是后台写线程或预读，与应用无关。作者用它在两个方向举证过：延迟问题源于文件系统 / 应用延迟时其实没有慢 I/O。

注意保留意见：同步 I/O 阻塞进程，但阻塞的若是后台 I/O 线程或预热逻辑，应用响应未必受影响——仍要看线程角色。

- BCC 版：跟踪 vfs_read/vfs_write 高层函数、内部过滤同步操作（跟踪全部再过滤，所以开销比想象大）；`fileslower [min_ms]`，0 = 打印全部（可能每秒几千条，慎用），默认 10ms；`-P PID`。
- bpftrace 版：直接 kprobe `new_sync_read`/`new_sync_write`（可能被内联导致探针失败——kprobe 不稳定接口的现实例子）。

## filetop —— I/O 工作负载定性

```
# filetop              # 默认 top20 按读字节排，1s 刷新
TID    COMM  READS WRITES R_KB W_KB T FILE
113962 java  15171 0     60684 0  R part-00903-...   ← 1 秒读 60MB，平均 4KB/次
```

- `-a`：包含 socket（T 列 S）/其他；默认只列普通文件（T=R）。
- 列：READS/WRITES 次数、R_KB/W_KB 字节、T 类型（R 普通文件 / S socket / O 其他）、FILE 名。
- 探针：kprobe vfs_read/vfs_write，inode 类型用 S_ISREG()/S_ISSOCK() 宏判断。
- 开销：VFS 高频时不可忽视（要读文件名，比别的工具更贵一点）。
- 选项：`-C` 不清屏滚动输出（留趋势）、`-r ROWS`、`-P PID`。
- 用法类比 top(1)：不是测量工具，是"发现意料之外的热点"的定性工具。

## writesync —— 同步写标志审计

```
@sync[dd,outfile]:100                  # dd oflag=sync 测试
@regular[tomcat-exec-142,tomcat_access.log]:15
```

kprobe vfs_write/writev，判断 `DT_REG` 且 `f_flags & O_DSYNC`（O_SYNC 内部也置 O_DSYNC）后按 (comm, 文件) 分桶到 @sync / @regular。

**原理**：同步写 = 写穿透（等存储完成，慢）；普通写 = 写回（进缓存即返回，快）。找出不必要带 sync 标志的写是大幅提速的低垂果实（书例 dd oflag=sync）。

## filetype —— 文件类型拆分

```
@[regular,vfs_read,make]:39600     # 编译负载以普通文件为主
@[fifo,vfs_read,sh]:15422          # shell 管道
@[socket,vfs_write,sshd]:435       # ssh 网络发送
```

tar | gzip 管道实例清晰地展示数据流：tar 读 REG → 写 FIFO；gzip 读 FIFO → 写 REG。

两个版本（同一目的的多种实现，作者刻意保留）：
- v1：`i_mode & 0xf000` 查表（0x8000 regular / 0x6000 block / 0x4000 directory / 0x2000 character / 0x1000 fifo / 0xc000 socket...，来自 uapi/linux/stat.h）
- v2：`(i_mode >> 12) & 15` 索引 DT_* 常量表（0=FIFO,2=CHR,4=DIR,6=BLK,8=REG,10=LNK,12=SOCK，来自 include/linux/fs.h）

## HFT 关联

- fileslower 是交易进程延迟排查的第一工具：任何 >10ms 的同步读写都值得解释。
- writesync 审计持久化路径：订单日志若误用 O_SYNC，每笔都等 fsync 落盘——改写回+批量 fsync 可提吞吐数量级。
- filetop 定位回测读热文件，配合 cachestat 决定是否加内存/预加载。

## 常见陷阱

- fileslower 只看同步 I/O：异步/后台写不在此列，别用它否定写放大问题。
- filetop 的高读文件先看平均尺寸（READS 与 R_KB 相除）再谈优化方向。

<details>
<summary>自测</summary>

1. 为什么 fileslower 的延迟比第 9 章磁盘工具的延迟更能证明应用受影响？什么时候例外？
2. O_DSYNC 与写回模式的性能差异机制是什么？writesync 如何区分两者？
3. filetype v1/v2 两个版本分别用什么掩码从 i_mode 提取类型？
</details>
