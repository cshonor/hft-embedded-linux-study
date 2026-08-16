# 8.5 可选练习

> 底本：《BPF之巅》第 8 章 文件系统，8.5 节（印刷 p359）。均可用 bpftrace 与 BCC 完成。

| # | 题目 | 考察点 / 提示 |
|---|---|---|
| 1 | 用 creat(2)/unlink(2) **系统调用跟踪点**重写 filelife | tracepoint 替代 kprobe 的改写能力 |
| 2 | 系统调用版 filelife 的优劣？ | 优：稳定接口；劣：只覆盖走系统调用的创建/删除，且 unlink(2)≠vfs_unlink（内部路径） |
| 3 | vfsstat 按文件系统分行输出（vfsstatx），ext4/TCP 各一行 | 综合 vfssize/fsrwstat 的 fs 类型读取（`file->f_inode->i_sb->s_type->name`） |
| 4 | 开发同时显示逻辑 I/O（VFS/FS 层）与物理 I/O（block 跟踪点）的工具 | 逻辑 vs 物理对比：vfsstat × 第 9 章块层 |
| 5 | 文件描述符泄漏分析工具（分配未释放的 FD） | 跟踪 alloc_fd() 与 close_fd() 配对 |
| 6 | （高级）按**挂载点**显示文件系统 I/O | 需从 file 结构体穿行到挂载信息 |
| 7 | （高级，未解决）页缓存**访问事件分布**工具 | 难点：mark_page_accessed 每秒百万次级，事件级跟踪开销爆炸——这正是 cachestat 只做聚合计数的原因 |

## 做题建议

- 第 1、2 题一组做完即可体会"跟踪点稳定但语义粗、kprobe 精确但脆弱"的核心权衡。
- 第 4 题就是本章与第 9 章的桥梁，做完等于打通逻辑/物理两层。
- 第 7 题作者标注未解决——它的答案本质上就是 cachestat 存在的理由：把事件分布降级为每秒聚合统计。
