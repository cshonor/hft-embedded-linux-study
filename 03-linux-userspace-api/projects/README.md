# TLPI 里程碑项目（projects/）

> 与 `chapter-NN-*` 学习笔记并排、互不打扰：学完触发章的笔记 → 停笔写对应项目 → 通过验收标准 → 再开新章。
> 文件夹命名规则：`{触发章号}-{项目名}`，按章号自然排序，和笔记目录的推进顺序完全一致。

## 检查点总览

状态：⬜ 未开始 ｜ 🔨 进行中 ｜ ✅ 完成

| 项目文件夹 | 触发章（学完开写） | 项目 | 核心知识点 | 状态 |
|---|---|---|---|---|
| [13-iobench](13-iobench/) | Ch13 文件 I/O 缓冲 | 三种 I/O 方式拷贝基准 | read/write、stdio 缓冲、O_DIRECT | ⬜ |
| [19-dirwatch](19-dirwatch/) | Ch19 监视文件事件 | 目录实时监控器 | inotify | ⬜ |
| [23-mytimeout](23-mytimeout/) | Ch23 定时器与睡眠 | 命令超时器 | timerfd、SIGCHLD、waitpid | ⬜ |
| [26-parallel](26-parallel/) | Ch26 监视子进程 | 并行任务执行器 | fork、waitpid(WNOHANG)、SIGCHLD | ⬜ |
| [28-msh](28-msh/) | Ch28 → Ch34 → Ch44 | mini-shell（三阶段） | exec、作业控制、管道 | ⬜ |
| [54-shmring](54-shmring/) | Ch49–55 内存映射与 POSIX IPC | SHM 无锁环形缓冲 + 延迟基准 | mmap、POSIX 信号量、文件锁、mlock | ⬜ |
| [63-t2t](63-t2t/) | Ch56–63 套接字与 epoll | tick-to-trade 三进程管线 | socket、epoll、timerfd、SHM | ⬜ |

## 使用规则

1. **触发即写**：到达触发章时，先不开新章，把对应项目的 v1 写完（每个项目 ≤ 2 个晚上）。
2. **验收才算完成**：每个项目 README 里的验收标准全部满足后，才能把状态改成 ✅。
3. **实测数字进 README**：涉及性能的项目（shmring、t2t）必须把 Pi 5 上测得的真实数字写进项目 README。
4. **msh 是唯一跨章项目**：v1（Ch28 fork/exec + 重定向）→ v2（Ch34 作业控制）→ v3（Ch44 多级管道），每次只加一个维度。

## 与笔记的关系

- 本目录**不放学习笔记**，笔记仍归 `chapter-NN-*` 各自管理。
- 每个项目做完后，如发现值得沉淀的通用结论，回写到对应 `chapter-NN-*` 的笔记里，项目里只留一句指向链接。
- `t2t`（Ch63 后的毕业项目）在 Pi 5 上联调，观测面用 bpftrace 从内核侧交叉验证应用侧延迟数字。
