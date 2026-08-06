# 第 8 章 UNIX 系统接口

**The UNIX System Interface**

## 本章讲什么

从第 7 章 **stdio** 下沉到 **fd + 系统调用**：`open`/`read`/`write`/`lseek`/`stat`、目录与链接，以及 **fopen 实现、目录列表、malloc 与 sbrk** 三个实例。是 Linux 用户态、自制类 Unix OS、HFT 低延迟 I/O 的核心一章。

## 学习重点（K&R 正文）

- **fd 0/1/2** 与 **FILE\*** 关系；**dup2** 重定向
- **read/write** 短读写循环；与 **fread** 缓冲/性能差异
- **open** 标志、**unlink/link** 与 inode 计数
- **lseek** 随机访问；**stat** 元数据
- 三实例：stdio 封装、目录枚举、堆分配

## 延伸：进程、IPC、信号（面试高频）

K&R 第二版本章侧重 **I/O**；下列接口是完整 UNIX 用户态模型必备，与你提纲 8.8–8.11 对应：

### pipe 管道

```c
int pipe(int fd[2]);   /* fd[0] 读端, fd[1] 写端 */
```

- **单向**字节流；**fork** 后父子各关一端实现 IPC
- Shell **`cmd1 | cmd2`**：`dup2` 把 stdout 接到 pipe 写端

### fork / exec / wait

| 调用 | 作用 |
|------|------|
| `fork()` | 复制进程；子返回 0，父返回 pid |
| `exec*` | 用新程序**替换**当前进程映像（path、argv、env） |
| `wait`/`waitpid` | 父进程回收子进程，避免**僵尸进程** |

- 现代内核 **写时复制（COW）**：fork 不立即复制全部物理页
- **fd 继承**：子进程复制 fd 表 → pipe、重定向基础

### signal 信号

```c
void (*signal(int sig, void (*handler)(int)))(int);
```

- **异步**通知：SIGINT（Ctrl+C）、SIGSEGV、SIGCHLD、定时器等
-  handler 中仅调 **async-signal-safe** 函数；复杂逻辑用 **`sigaction`** + 标志位
- 易打乱执行流：竞态、可重入 —— 重难点

## 场景映射

| 方向 | 本章技能 |
|------|----------|
| 自制 OS / 类 Unix | fd、文件、目录、进程模型设计参考 |
| HFT | 无缓冲 read/write；pipe 转发；dup 分离日志 |
| 嵌入式 Linux | 设备文件 fd；fork+pipe 采集进程 |

## 重难点

1. stdio 缓冲 vs 原生 read/write  
2. fork 后 fd 共享、COW  
3. pipe 单向 + 同步  
4. 硬链接 / unlink 与 inode  
5. 信号异步安全  

## 小节

- [8.1 文件描述符](./8.1-文件描述符.md)
- [8.2 低级 I/O：read 和 write](./8.2-低级IO-read和write.md)
- [8.3 open、creat、close 和 unlink](./8.3-open-creat-close和unlink.md)
- [8.4 随机访问：lseek](./8.4-随机访问-lseek.md)
- [8.5 实例：fopen 和 getc 的实现](./8.5-实例-fopen和getc函数的实现.md)
- [8.6 实例：目录列表](./8.6-实例-目录列表.md)
- [8.7 实例：存储分配程序](./8.7-实例-存储分配程序.md)
