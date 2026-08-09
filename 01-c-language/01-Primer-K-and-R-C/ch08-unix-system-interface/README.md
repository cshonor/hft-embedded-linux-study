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

---

## 章节自测

> 从 stdio 下沉到 fd + syscall。内核、HFT、自制 OS 的核心一章。看代码 → 想答案 → 点开验证。

### Q1: 文件描述符

```c
// fd 0/1/2 分别是什么？
// 以下程序输出什么？

int fd = open("test.txt", O_WRONLY | O_CREAT, 0644);
printf("fd = %d\n", fd);
write(fd, "hello", 5);
close(fd);

int fd2 = open("test2.txt", O_WRONLY | O_CREAT, 0644);
printf("fd2 = %d\n", fd2);
```

> `fd` 和 `fd2` 各是多少？为什么不是 0 或 1？

<details>
<summary>答案与复习指引</summary>

**答案：** `fd` = 3，`fd2` = 3（fd 回收复用）

**解析：** 进程启动时自动打开三个 fd：
- 0 = stdin（标准输入）
- 1 = stdout（标准输出）
- 2 = stderr（标准错误）

`open` 返回最小可用 fd → 第一个新 fd 是 3。`close(fd)` 后 fd 3 被回收，下次 `open` 复用 3。

**fd 生命周期：** `fork` 后子进程**继承**父的 fd 表（COW）→ 管道和重定向的基础。

**复习：** → [8.1 文件描述符](./8.1-文件描述符.md)

</details>

### Q2: read/write 短读写

```c
char buf[100];
int n;
int fd = open("large.bin", O_RDONLY);

// 这样写有什么问题？
n = read(fd, buf, 100);
// 假设 buf 只装了一部分，n < 100 正常吗？

printf("read %d bytes\n", n);
```

> `read` 返回 100 以下的值是错误吗？什么情况下 `read` 返回少于请求的字节？

<details>
<summary>答案与复习指引</summary>

**答案：** 不是错误。`read` 返回**实际读到的字节数**，可能少于请求量。

**短读写原因：**
1. 到达文件末尾（EOF）→ 返回剩余字节数
2. 读终端/管道 → 一行结束就返回
3. 被信号中断（`EINTR`）→ 返回已读量
4. 网络套接字 → 数据分片到达

**正确写法（短读写循环）：**
```c
ssize_t total = 0;
while (total < want) {
    ssize_t n = read(fd, buf + total, want - total);
    if (n < 0) { if (errno == EINTR) continue; break; }
    if (n == 0) break; // EOF
    total += n;
}
```

**教训：** 永远不能假设 `read(fd, buf, 100)` 一次返回 100。

**复习：** → [8.2 低级IO-read和write](./8.2-低级IO-read和write.md)

</details>

### Q3: open 标志

```c
// (1)
int fd1 = open("file.txt", O_WRONLY);
// (2)
int fd2 = open("file.txt", O_WRONLY | O_CREAT, 0644);
// (3)
int fd3 = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
// (4)
int fd4 = open("file.txt", O_WRONLY | O_APPEND);
```

> 四种方式各有什么区别？哪种最常用？

<details>
<summary>答案与复习指引</summary>

| 写法 | 文件不存在时 | 文件存在时 |
|------|------------|-----------|
| (1) `O_WRONLY` | **失败** | 打开，从头覆写 |
| (2) `+O_CREAT` | 创建 | 打开，从头覆写（不清空！） |
| (3) `+O_TRUNC` | 创建 | 清空后打开 |
| (4) `O_APPEND` | **失败** | 追加到末尾 |

**最常用：** (3) 新建/覆盖文件用 `O_WRONLY | O_CREAT | O_TRUNC`，日志追加用 `O_WRONLY | O_APPEND`。

**HFT 场景：** 日志文件用 `O_APPEND` + `O_WRONLY`，多进程追加时 `O_APPEND` 保证原子性（不会交错写入）。

**第三参数 `0644`** = 文件权限（`rw-r--r--`），仅在 `O_CREAT` 时生效。被 `umask` 修正后实际为 `0644 & ~umask`。

**复习：** → [8.3 open、creat、close和unlink](./8.3-open-creat-close和unlink.md)

</details>

### Q4: lseek 随机访问

```c
int fd = open("data.bin", O_RDONLY);

lseek(fd, 100, SEEK_SET);    // (1) 从头偏移 100
lseek(fd, 50, SEEK_CUR);     // (2) 从当前再偏 50
lseek(fd, -10, SEEK_END);    // (3) 从末尾回退 10
off_t pos = lseek(fd, 0, SEEK_CUR);  // (4) 查当前位置
```

> 四行各做什么？`(4)` 返回什么？lseek 能用于管道吗？

<details>
<summary>答案与复习指引</summary>

**答案：**
1. `SEEK_SET` — 从文件起始偏移 100 → 位置 = 100
2. `SEEK_CUR` — 从当前位置偏移 50 → 位置 = 150
3. `SEEK_END` — 从文件末尾回退 10 → 位置 = 文件大小 - 10
4. `SEEK_CUR` 偏移 0 → 返回当前偏移量（150... 等，是 文件大小-10）

**lseek 不能用于管道/套接字/FIFO：** 这些流没有"位置"概念，`lseek` 返回 -1 + `errno = ESPIPE`。

**HFT 用途：** 大型 tick 数据库按时间戳定位、固定大小记录的随机读取。

**注意：** `lseek` 只移动**文件偏移量**，不做实际 I/O。

**复习：** → [8.4 随机访问-lseek](./8.4-随机访问-lseek.md)

</details>

### Q5: stdio 缓冲 vs 原生 I/O

```c
// 方式 A: stdio
FILE *fp = fopen("log.txt", "w");
for (int i = 0; i < 10000; i++)
    fprintf(fp, "line %d\n", i);
fclose(fp);

// 方式 B: 原生 I/O
int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
for (int i = 0; i < 10000; i++) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "line %d\n", i);
    write(fd, buf, len);  // 每次 syscall
}
close(fd);
```

> 哪种方式更快？为什么？HFT 里选哪种？

<details>
<summary>答案与复习指引</summary>

**答案：** 方式 A（stdio）快得多。

**解析：**
- `fprintf` → 写入 stdio 内部缓冲区（用户态），缓冲区满或 `fclose` 时才调 `write` syscall
- 每次 `write` → 独立 syscall（用户态→内核态切换），10000 次 syscall 非常慢

**性能差距：** 在 10000 次小写入的场景下，stdio 可以快 10-100 倍。

**HFT 选择：**
- 需要缓冲批量写 → stdio（`fprintf` + 偶尔 `fflush`）
- 需要绝对无延迟 → 原生 `write`（自己管理缓冲，单次 syscall 批量写）
- 不能接受任何缓冲延迟 → `write` 直接写 fd

**教训：** syscall 比函数调用贵 3-4 个数量级。能用缓冲就用缓冲。

**复习：** → [8.5 实例-fopen和getc函数的实现](./8.5-实例-fopen和getc函数的实现.md) · [8.2 低级IO-read和write](./8.2-低级IO-read和write.md)
