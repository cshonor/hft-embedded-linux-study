# TLPI 第 13 章 — File I/O Buffering

> 对应目录：`chapter-13-file-io-buffering/`  
> 书名原文：**File I/O Buffering**  
> ⚠️ **两层缓冲：** 内核页缓存（`read`/`write`）+ stdio 用户态缓冲（`FILE*`）。混用必乱序。

**优先级**：🔴（日志 / 持久化 / 高性能 IO / DB）  
**前置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/notes.md) · [Ch5 Further I/O](../chapter-05-file-io-further/notes.md) · [Ch12 `/proc`](../chapter-12-system-process-info/notes.md)  
**后置**：[Ch14 File Systems](../chapter-14-file-systems/notes.md) · [Ch49 mmap](../chapter-49-memory-mappings/notes.md) · [Ch63 替代 I/O](../chapter-63-alternative-io/notes.md)

---

## 章节目标

厘清两层缓冲与延迟写；掌握 `fsync`/`fdatasync`/`O_SYNC`；会控 stdio 缓冲与 `fflush`；**严禁无保护地混用 stdio 与原生 read/write**；了解 `posix_fadvise` / `O_DIRECT`。

---

## 13.1 内核缓冲（Buffer / Page Cache）

`read()` / `write()` **不直接对磁盘**做同步块传输，而是在用户缓冲 ↔ **内核页缓存**之间拷贝。

### 延迟写（Delayed Write）

| | |
|--|--|
| `write()` 成功返回 | **≠ 已落盘**；数据多在脏页里 |
| 好处 | 合并小写、少寻道、吞吐高 |
| 风险 | 崩溃/断电 → 脏页丢失 |

后台 flusher 周期性刷脏页；参数见 `/proc/sys/vm/dirty_*`。

### 强制落盘

```c
#include <unistd.h>
void sync(void);          /* 全局刷脏；标准允许不等待完成；业务勿狂调 */
int fsync(int fd);        /* 该文件：数据 + 元数据；等到设备完成 */
int fdatasync(int fd);    /* 优先刷数据；元数据按需；通常比 fsync 轻 */
```

### `open` 同步标志

| 标志 | 近似语义 |
|------|----------|
| `O_SYNC` | 每次 `write` ≈ `fsync`（数据+元数据） |
| `O_DSYNC` | 每次 `write` ≈ `fdatasync` |

> 同步 IO **极慢**；事务/关键日志按需用。  
> 若前面还有 **stdio**，必须先 `fflush` 再依赖 `O_SYNC`/`write`。

---

## 13.2 stdio 用户态缓冲（`FILE*`）

`printf` / `fwrite` / `fgets` 先打用户缓冲；满或刷新时才 `read`/`write`。

### `setvbuf` 三种模式

```c
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
```

| mode | 行为 | 典型默认 |
|------|------|----------|
| `_IOFBF` | 全缓冲：满才刷 | 普通磁盘文件 |
| `_IOLBF` | 行缓冲：`\n` 或满 | **终端上的 stdout** |
| `_IONBF` | 无缓冲 | **stderr** |

### 刷新

```c
int fflush(FILE *stream);   /* NULL = 刷新所有输出流 */
```

自动刷新：`fclose`、正常退出。  
> **不要**依赖 `fflush` 丢弃**输入**缓冲（未标准化）。

**重定向坑：** stdout 接到管道/文件时，常从行缓冲变成**全缓冲** → 无 `\n`/`fflush` 时日志「卡住」。

Demo：[`code/stdio_buffering.c`](./code/stdio_buffering.c)

---

## 13.3 【致命】混用 `FILE*` 与 `read`/`write`(fd)

两套缓冲**互不可见**：

- stdio 有自己的用户缓冲  
- `read`/`write` 直达内核缓存，**绕过** stdio  

→ 乱序、丢数据。

| 做法 | |
|------|--|
| ✅ | 全程只用一套接口 |
| ✅ | 必须混用：切换前对输出流 **`fflush`** |
| ❌ | `printf` 完直接对同一 fd `write`（未 fflush） |

Demo：[`code/mix_stdio_write.c`](./code/mix_stdio_write.c)

---

## 13.4 `posix_fadvise` — 访问模式提示

```c
int posix_fadvise(int fd, off_t offset, off_t len, int advice);
```

仅**建议**内核；常用：

| advice | 含义 |
|--------|------|
| `POSIX_FADV_NORMAL` | 默认 |
| `POSIX_FADV_SEQUENTIAL` | 顺序 → 加大预读 |
| `POSIX_FADV_RANDOM` | 随机 → 少预读 |
| `POSIX_FADV_DONTNEED` | 短期不用 → 可回收缓存 |
| `POSIX_FADV_WILLNEED` | 即将用 → 预加载 |

---

## 13.5 Direct I/O（`O_DIRECT`）

`open(path, O_RDWR | O_DIRECT)`：绕过页缓存，应用与块设备更直接传数据。

### 对齐（硬约束）

缓冲区地址、文件偏移、传输长度须为**逻辑块大小**（常见 512/4096）整数倍。  
缓冲分配：`posix_memalign()`。

| 适用 | 自带缓存的 DB 等，不想占系统 page cache |
|------|----------------------------------------|
| 缺点 | 小 IO 很惨；**不能替代 `fsync` 持久化** |

> **`O_DIRECT` ≠ `O_SYNC`**：前者绕过页缓存；设备本身可能有缓存，落盘仍要 `fsync`/`fdatasync`。

Demo：[`code/odirect_align.c`](./code/odirect_align.c)

---

## 13.6 两层数据流

**写：** 应用 → stdio 用户缓冲 →（`fflush`）→ `write` → 内核脏页 →（`fsync`/flusher）→ 磁盘  

**读：** 磁盘 → 页缓存 → `read` → stdio 缓冲 → `fgets`/… → 应用  

---

## 13.7 速查：持久化相关标志/调用

| 机制 | 作用范围 | 等不等落盘 | 备注 |
|------|----------|------------|------|
| `write` | → 页缓存 | 否 | 成功 ≠ 落盘 |
| `fsync` | 一文件数据+元数据 | 是 | 最稳、最贵 |
| `fdatasync` | 优先数据 | 是 | 常够用 |
| `sync` | 全局 | 不定 | 业务禁用狂调 |
| `O_SYNC` | 每次 write≈fsync | 是 | 极慢 |
| `O_DSYNC` | 每次 write≈fdatasync | 是 | |
| `O_DIRECT` | 绕过页缓存 | **否** | 仍要 fsync 才谈持久化 |
| `fflush` | **仅 stdio 用户缓冲** | 否 | 不刷磁盘 |

---

## 13.8 易错清单

1. `write` 成功 ≠ 落盘  
2. 只要数据、不要改大小/mtime → 优先 `fdatasync`  
3. stdout→文件/管道：全缓冲，无 `\n`/`fflush` 看不见输出  
4. `O_SYNC` 管的是 `write`，管不住未 `fflush` 的 stdio  
5. `O_DIRECT` 对齐失败 → `EINVAL`；用 `posix_memalign`  
6. 崩溃丢：**用户态 stdio** + **内核脏页**；已刷盘的安全  

---

## 练习

1. 终端 vs 重定向：stdout 行缓冲/全缓冲 + `fflush`  
2. 小循环多次 `write` vs 加大缓冲/批写  
3. 复现 `printf`+`write` 乱序，用 `fflush` 修  
4. `O_DIRECT` 故意不对齐 → `EINVAL`  
5. （选）`posix_fadvise` 顺序读提示  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 两层：内核页缓存 + stdio 用户缓冲 |
| 2 | `write` ≠ 落盘；要持久化用 `fsync`/`fdatasync` |
| 3 | 混用 stdio 与 read/write 前必须 `fflush` |
| 4 | 重定向后 stdout 常变全缓冲 |
| 5 | `O_DIRECT` 绕缓存≠持久化；要对齐 |
| 6 | `fdatasync` 通常够；`sync()` 勿滥用 |

---

## 参考

- Kerrisk · TLPI Ch13  
- `man 2 fsync` · `man 2 fdatasync` · `man 2 open`（`O_SYNC`/`O_DIRECT`）· `man 3 setvbuf` · `man 3 posix_fadvise`
