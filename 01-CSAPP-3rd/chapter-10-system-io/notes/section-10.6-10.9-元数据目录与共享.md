## 10.6–10.9 元数据、目录与共享

### 10.6 读取文件元数据

```c
int stat(const char *pathname, struct stat *buf);
int fstat(int fd, struct stat *buf);
```

常用字段：

| 字段 | 含义 |
|------|------|
| `st_mode` | 类型 + 权限 |
| `st_size` | 字节大小 |
| `st_mtime` | 修改时间 |

```c
S_ISREG(st_mode)  // 普通文件
S_ISDIR(st_mode)  // 目录
```

**HFT：** 启动时 `stat` 配置文件；**mmap 前** 知 `st_size`；热路径避免 `stat`。

### 10.7 读取目录内容

```c
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
```

- 扫描配置目录、日志轮转 — 非 tick 路径

### 10.8 共享文件

三层结构：

```
进程 fd 表 → 打开文件表 → v-node 表（inode 内容）
```

| 机制 | 效果 |
|------|------|
| 两 fd 同 **打开文件表项** | 共享 **文件偏移** |
| `fork` | 父子共享已打开文件的偏移 |
| 不同 `open` 同一路径 | 通常 **独立偏移** |

- **`O_APPEND`** — 写前内核把偏移设到末尾，原子追加

### 10.9 I/O 重定向

```c
int dup(int oldfd);
int dup2(int oldfd, int newfd);
```

- **shell `> file`** — `dup2(fd, STDOUT_FILENO)` 把 stdout 指到文件
- 理解 **fd 编号可复用** — `close` 后最小可用号

**HFT：** 守护进程 **重定向 stdin/stdout 到 /dev/null**；日志 fd 单独管理。

---

← [本章导读](../README.md)
