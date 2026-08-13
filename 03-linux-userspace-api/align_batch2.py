#!/usr/bin/env python3
"""Batch 2: Align Ch6-Ch14 notes to book section structure."""
import os

BASE = 'C:/Users/12392/Desktop/hft/03-linux-userspace-api'

def move(ch_dir, old, new):
    old_fp = os.path.join(BASE, ch_dir, 'notes', old)
    new_fp = os.path.join(BASE, ch_dir, 'notes', new)
    if os.path.exists(old_fp) and old != new:
        os.rename(old_fp, new_fp)
        print(f"  RENAMED: {old} -> {new}")

def delete(ch_dir, name):
    fp = os.path.join(BASE, ch_dir, 'notes', name)
    if os.path.exists(fp):
        os.remove(fp)
        print(f"  DELETED: {name}")

def create(ch_dir, filename, content):
    fp = os.path.join(BASE, ch_dir, 'notes', filename)
    with open(fp, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"  CREATED: {filename}")

SKELETON = lambda ch, sec, title, body: f"""# {sec} {title}

> 本章：[TLPI 第 {ch} 章](./README.md)

## 本节讲什么

{body}

## 与后续衔接

---

## 代码自测

<details>
<summary>参考答案</summary>

</details>
"""

# ============================================================
# Ch6: Processes (10 sections, 6 existing)
# ============================================================
print("=== Chapter 6 ===")
ch6 = 'chapter-06-processes'
# 6.1 ✓, 6.2 ✓, 6.3 ✓ stay
# 6.4-argv → 6.6, 6.5-environment → 6.7, 6.6-setjmp-longjmp → 6.8
move(ch6, '6.4-argv.md', '6.6-command-line-args.md')
move(ch6, '6.5-environment.md', '6.7-environment-list.md')
move(ch6, '6.6-setjmp-longjmp.md', '6.8-setjmp-longjmp.md')
# Create missing
create(ch6, '6.4-virtual-memory.md', SKELETON(6, '6.4', 'Virtual Memory Management', """Linux 采用虚拟内存管理：每个进程有独立的虚拟地址空间，通过页表映射到物理内存。

### 核心概念
- **虚拟页 → 物理页帧**：MMU 通过页表转换
- **缺页中断**：访问未映射的页时，内核分配物理页
- **按需分页（demand paging）**：页在首次访问时才加载
- **交换（swap）**：内存不足时将页写到交换分区

### 虚拟内存区域（VMA）
内核为每个进程维护 VMA 列表，记录各段（text/data/heap/stack/mapping）的属性。
- `/proc/[pid]/maps` 查看进程的 VMA 布局
- `/proc/[pid]/smaps` 更详细，含 RSS/PSS

### HFT 关注点
- `mlockall()` 锁定内存，防止换页导致延迟尖峰
- 大页（huge pages）减少 TLB miss"""))

create(ch6, '6.5-stack-frames.md', SKELETON(6, '6.5', 'The Stack and Stack Frames', """每次函数调用在栈上创建一个栈帧（stack frame），包含局部变量、参数、返回地址。

### 栈帧结构
```
高地址 │ 返回地址     │
       │ 旧 rbp       │ ← rbp 指向这里
       │ 局部变量     │
       │ 保存的寄存器 │
低地址 │ ...          │ ← rsp 指向栈顶
```

### 栈的特性
- 自动管理：函数返回时栈帧自动释放
- 方向：x86-64 上栈向低地址增长
- 大小：默认 8MB（`ulimit -s`），超限 → SIGSEGV
- 栈溢出常见原因：递归太深、大数组局部变量

### 栈 vs 堆
| 特性 | 栈 | 堆 |
|------|----|----|
| 分配速度 | 极快（移动 sp） | 慢（malloc 搜索空闲块） |
| 生命周期 | 函数返回自动释放 | 手动 free |
| 大小 | 有限（默认 8MB） | 大（受虚拟地址空间限制） |"""))

create(ch6, '6.9-summary.md', SKELETON(6, '6.9', 'Summary', """本章回顾进程的基本概念。

| 节 | 主题 | 关键 API |
|----|------|---------|
| 6.1 | 进程 vs 程序 | getpid()/getppid() |
| 6.2 | PID/PPID | getpid()/getppid() |
| 6.3 | 内存布局 | /proc/[pid]/maps |
| 6.4 | 虚拟内存 | mmap/mlock |
| 6.5 | 栈帧 | (理解概念) |
| 6.6 | 命令行参数 | argc/argv |
| 6.7 | 环境变量 | getenv/setenv |
| 6.8 | 非局部跳转 | setjmp/longjmp |"""))

create(ch6, '6.10-exercises.md', SKELETON(6, '6.10', 'Exercises', """**1.** 编写程序打印自己的 PID 和 PPID。

**2.** 使用 setjmp/longjmp 实现一个简单的错误恢复机制。

**3.** 解释为什么在信号处理器中调用 longjmp 是危险的。"""))

# ============================================================
# Ch7: Memory Allocation (4 sections, 7 existing → merge)
# ============================================================
print("\n=== Chapter 7 ===")
ch7 = 'chapter-07-memory-allocation'
# Merge 7.1+7.2 → 7.1, 7.3+7.4+7.5+7.6+7.7 → keep in 7.1
# Book only has: 7.1 heap, 7.2 alloca, 7.3 summary, 7.4 exercises
# 7.1-program-break and 7.2-brk-sbrk → merge into 7.1
# 7.3-malloc-free etc → also 7.1 (heap allocation)

# Read and merge existing files for 7.1
parts = []
for fn in ['7.1-program-break.md', '7.2-brk-sbrk.md', '7.3-malloc-free.md', '7.4-calloc-realloc.md', '7.5-malloc.md', '7.6-glibc.md', '7.7-memory.md']:
    fp = os.path.join(BASE, ch7, 'notes', fn)
    if os.path.exists(fp):
        with open(fp, 'r', encoding='utf-8') as f:
            parts.append(f.read())
        delete(ch7, fn)

merged = parts[0]
for p in parts[1:]:
    merged += '\n\n---\n\n' + p
with open(os.path.join(BASE, ch7, 'notes', '7.1-heap-allocation.md'), 'w', encoding='utf-8') as f:
    f.write(merged)
print("  CREATED: 7.1-heap-allocation.md (merged 7 files)")

create(ch7, '7.2-alloca.md', SKELETON(7, '7.2', 'Allocating Memory on the Stack: alloca()', """`alloca()` 在栈上分配内存，函数返回时自动释放。

### 原型
```c
void *alloca(size_t size);
```

### 特性
- 分配在栈帧上，函数返回自动释放
- 不需要 free，不会内存泄漏
- 速度极快（仅调整 sp）
- 分配大小受栈限制（默认 8MB）

### vs malloc
| 特性 | alloca | malloc |
|------|--------|--------|
| 分配位置 | 栈 | 堆 |
| 释放方式 | 函数返回自动 | 手动 free |
| 速度 | 极快 | 较慢 |
| 大小限制 | 栈大小 | 虚拟地址空间 |

### 变长数组（C99）
C99 的变长数组（VLA）本质等同于 alloca，也在栈上分配。
```c
int n = 10;
int arr[n];  // VLA，栈上分配
```

### 注意
- alloca 不释放内存，只是调整 sp → 不要在循环中大量调用
- 信号处理器中用 alloca 可能导致栈溢出"""))

create(ch7, '7.3-summary.md', SKELETON(7, '7.3', 'Summary', """本章回顾内存分配。

| 分配方式 | API | 位置 | 释放 |
|---------|-----|------|------|
| 堆分配 | malloc/calloc/realloc | 堆 | free |
| 栈分配 | alloca | 栈 | 自动 |
| 系统调用 | brk/sbrk | 堆顶 | sbrk(-n) |
| 内存映射 | mmap | 任意 | munmap |"""))

create(ch7, '7.4-exercises.md', SKELETON(7, '7.4', 'Exercises', """**1.** 实现 `malloc` 的简化版（用 sbrk 维护空闲链表）。

**2.** 解释 `calloc` 和 `malloc + memset` 的区别。

**3.** 为什么 `realloc(ptr, 0)` 的行为是未定义的？"""))

# ============================================================
# Ch8: Users and Groups (7 sections, 6 existing)
# ============================================================
print("\n=== Chapter 8 ===")
ch8 = 'chapter-08-users-and-groups'
# 8.1-uid-gid → delete (basic concept, covered in ch2)
# 8.2-etc-passwd → 8.1
# 8.4-etc-shadow-root → 8.2
# 8.3-etc-group → 8.3
# 8.6-supplementary-groups → 8.4 (retrieving info)
# 8.5-crypt → 8.5
# Missing: 8.6 Summary, 8.7 Exercises
delete(ch8, '8.1-uid-gid.md')
move(ch8, '8.2-etc-passwd.md', '8.1-passwd-file.md')
move(ch8, '8.4-etc-shadow-root.md', '8.2-shadow-file.md')
move(ch8, '8.3-etc-group.md', '8.3-group-file.md')
move(ch8, '8.6-supplementary-groups.md', '8.4-retrieving-info.md')
move(ch8, '8.5-crypt.md', '8.5-password-encryption.md')
create(ch8, '8.6-summary.md', SKELETON(8, '8.6', 'Summary', """本章回顾用户和组。

| 文件 | 用途 | 权限 |
|------|------|------|
| /etc/passwd | 用户基本信息 | 全局可读 |
| /etc/shadow | 密码哈希 | 仅 root |
| /etc/group | 组信息 | 全局可读 |

### 关键 API
- `getpwuid()`/`getpwnam()` — 查询用户信息
- `getgrgid()`/`getgrnam()` — 查询组信息
- `getgroups()`/`setgroups()` — 附属组"""))
create(ch8, '8.7-exercises.md', SKELETON(8, '8.7', 'Exercises', """**1.** 编写程序，给定 UID 打印用户名和主组名。

**2.** 解释为什么 /etc/shadow 的引入是必要的。"""))

# ============================================================
# Ch9: Process Credentials (9 sections, 7 existing)
# ============================================================
print("\n=== Chapter 9 ===")
ch9 = 'chapter-09-process-credentials'
# 9.1-credentials → 9.1 (Real UID/GID)
# 9.2-set-user-id → 9.3 (Set-User-ID Programs)
# 9.4-saved-id → 9.4 ✓
# 9.5-api → 9.7 (Retrieving and Modifying)
# 9.6-credentials → merge into 9.7
# 9.8-fork-exec → extra, delete or keep as part of 9.7
# 9.9-section-9-9 → extra, delete
# Missing: 9.2 (Effective UID/GID), 9.5 (FS UID/GID), 9.6 (Supplementary Groups), 9.8 (Summary), 9.9 (Exercises)

move(ch9, '9.1-credentials.md', '9.1-real-uid-gid.md')
move(ch9, '9.2-set-user-id.md', '9.3-set-user-id.md')
# 9.4 stays
# Merge 9.5-api + 9.6-credentials → 9.7
with open(os.path.join(BASE, ch9, 'notes', '9.5-api.md'), 'r', encoding='utf-8') as f:
    c_95 = f.read()
with open(os.path.join(BASE, ch9, 'notes', '9.6-credentials.md'), 'r', encoding='utf-8') as f:
    c_96 = f.read()
with open(os.path.join(BASE, ch9, 'notes', '9.7-api.md'), 'w', encoding='utf-8') as f:
    f.write(c_95 + '\n\n---\n\n' + c_96)
print("  CREATED: 9.7-api.md (merged 9.5+9.6)")
delete(ch9, '9.5-api.md')
delete(ch9, '9.6-credentials.md')
delete(ch9, '9.8-fork-exec.md')
delete(ch9, '9.9-section-9-9.md')

create(ch9, '9.2-effective-uid-gid.md', SKELETON(9, '9.2', 'Effective User ID and Effective Group ID', """Effective UID/GID 决定进程的权限检查。

### Real vs Effective
- **Real UID**：启动进程的用户
- **Effective UID**：权限检查使用的 UID
- 通常 real == effective
- set-user-ID 程序：real != effective

### 示例
```bash
ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root ...  # set-user-ID bit (s)
```
运行 passwd 时：real=普通用户，effective=root → 可以写 /etc/shadow

### 相关 API
- `geteuid()`/`getegid()` — 获取 effective ID
- `seteuid()`/`setegid()` — 设置 effective ID"""))

create(ch9, '9.5-fs-uid-gid.md', SKELETON(9, '9.5', 'File-System User ID and File-System Group ID', """Linux 特有的 FS UID/GID，用于文件系统权限检查。

### 为什么需要 FS UID
- Linux 历史上 NFS 需要区分 effective UID 和文件系统 UID
- FS UID 默认 = effective UID
- `setfsuid()`/`setfsgid()` 修改（仅 Linux）

### 现代系统
- FS UID 基本被废弃，始终等于 effective UID
- 了解概念即可，不需要主动使用"""))

create(ch9, '9.6-supplementary-groups.md', SKELETON(9, '9.6', 'Supplementary Group IDs', """附属组：用户除了主组外还可以属于多个组。

### 相关 API
- `getgroups()` — 获取附属组列表
- `setgroups()` — 设置附属组（需特权）
- `initgroups()` — 从 /etc/group 加载用户的附属组

### 权限检查顺序
1. 如果 effective UID == 0 (root) → 允许
2. 检查文件 owner 与 effective UID
3. 检查文件 group 与 effective GID + 附属组
4. 检查 other 权限"""))

create(ch9, '9.8-summary.md', SKELETON(9, '9.8', 'Summary', """本章回顾进程凭证。

| 凭证 | 说明 | 修改 API |
|------|------|---------|
| Real UID/GID | 启动用户 | setuid()/setgid() |
| Effective UID/GID | 权限检查 | seteuid()/setegid() |
| Saved set-UID/GID | exec 保存 | (通过 setuid 间接) |
| FS UID/GID | Linux 特有 | setfsuid() (已废弃) |
| Supplementary groups | 附属组 | setgroups() |"""))

create(ch9, '9.9-exercises.md', SKELETON(9, '9.9', 'Exercises', """**1.** 编写 set-user-ID 程序，临时提升权限写文件后立即 drop privilege。

**2.** 解释 real/effective/saved UID 在 fork 和 exec 时的传递规则。"""))

# ============================================================
# Ch10: Time (9 sections, 1 existing)
# ============================================================
print("\n=== Chapter 10 ===")
ch10 = 'chapter-10-time'
# Only 10.3-section-10-3 exists, need to create 8 more
move(ch10, '10.3-section-10-3.md', '10.3-timezones.md')
for sec, title, body in [
    ('10.1', 'Calendar Time', """日历时间：从 Epoch(1970-01-01 00:00:00 UTC) 起的秒数。

### API
- `time()` — 秒级精度
- `gettimeofday()` — 微秒级精度（已废弃，推荐 clock_gettime）
- `clock_gettime(CLOCK_REALTIME, &ts)` — 纳秒级精度

### timespec 结构
```c
struct timespec {
    time_t   tv_sec;    // 秒
    long     tv_nsec;   // 纳秒 [0, 999999999]
};
```"""),
    ('10.2', 'Time-Conversion Functions', """将 time_t 转换为可读格式。

### 转换链
```
time_t → struct tm → 字符串
  ctime()    localtime()   asctime()/strftime()
  gmtime()
```

### 关键函数
- `ctime(&t)` — 直接转为 "Wed Aug 13 08:00:00 2025\n"
- `localtime(&t)` — 转为本地时区的 struct tm
- `gmtime(&t)` — 转为 UTC 的 struct tm
- `strftime(buf, sz, "%Y-%m-%d %H:%M:%S", &tm)` — 格式化"""),
    ('10.4', 'Locales', """Locale 控制程序的语言/地区设置。

### 设置
```c
setlocale(LC_ALL, "");      // 从环境变量设置
setlocale(LC_ALL, "C");     // 默认 C locale
setlocale(LC_ALL, "zh_CN.UTF-8");
```

### 类别
- LC_TIME — 日期时间格式
- LC_MONETARY — 货币格式
- LC_COLLATE — 字符串比较
- LC_CTYPE — 字符分类"""),
    ('10.5', 'Updating the System Clock', """设置系统时钟需要特权（CAP_SYS_TIME）。

### API
- `settimeofday()` — 设置时钟（已废弃）
- `clock_settime(CLOCK_REALTIME, &ts)` — 推荐方式
- `adjtime()` — 微调时钟（NTP 使用）"""),
    ('10.6', 'The Software Clock (Jiffies)', """内核软件时钟的分辨率。

- 内核通过定时器中断（HZ）驱动调度和计时
- `HZ=250`（默认）：每秒 250 次中断，分辨率 4ms
- `CONFIG_HZ` 编译时配置
- `clock_getres()` 查询时钟分辨率"""),
    ('10.7', 'Process Time', """进程消耗的 CPU 时间。

### 两种 CPU 时间
- **user time**：用户态执行时间
- **system time**：内核态执行时间

### API
- `times()` — 获取进程及其子进程的 user/system time
- `clock()` — 简化的进程时间（CLOCK_PROCESS_CPUTIME_ID）
- `clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts)` — 高精度

### time 命令
```
$ time ./a.out
real    0m1.234s   # 实际流逝
user    0m0.500s   # 用户态 CPU
sys     0m0.200s   # 内核态 CPU
```"""),
    ('10.8', 'Summary', """本章回顾时间 API。

| 时间类型 | API | 精度 |
|---------|-----|------|
| 日历时间 | time()/clock_gettime() | 秒~纳秒 |
| 时间转换 | localtime()/strftime() | - |
| 进程时间 | times()/clock() | 时钟滴答 |
| 定时器 | timer_create()/timerfd | 纳秒 |"""),
    ('10.9', 'Exercise', """**1.** 编写程序测量一段代码的执行时间（用 clock_gettime(CLOCK_MONOTONIC)）。

**2.** 解释 CLOCK_REALTIME 和 CLOCK_MONOTONIC 的区别。"""),
]:
    create(ch10, f'{sec}-{title.lower().replace(" ", "-").replace("(", "").replace(")", "").replace("/", "-").replace(",", "")}.md', SKELETON(10, sec, title, body))

# ============================================================
# Ch11: System Limits and Options (7 sections, 7 existing)
# ============================================================
print("\n=== Chapter 11 ===")
ch11 = 'chapter-11-system-limits'
# 7 existing, 7 book sections — just renumber
move(ch11, '11.1-concepts.md', '11.1-system-limits.md')
move(ch11, '11.2-api.md', '11.2-runtime-limits.md')
move(ch11, '11.3-limits-unistd.md', '11.3-file-related-limits.md')
move(ch11, '11.4-limits.md', '11.4-indeterminate-limits.md')
move(ch11, '11.5-indeterminate.md', '11.5-system-options.md')
move(ch11, '11.6-feature-options.md', '11.6-summary.md')
move(ch11, '11.7-section-11-7.md', '11.7-exercises.md')

# ============================================================
# Ch12: System and Process Information (4 sections, 9 existing)
# ============================================================
print("\n=== Chapter 12 ===")
ch12 = 'chapter-12-system-process-info'
# Book only has 4 sections: 12.1 /proc, 12.2 uname(), 12.3 Summary, 12.4 Exercises
# Merge 12.1-uname → 12.2, 12.2-proc+12.3-proc → 12.1, others merge/delete
with open(os.path.join(BASE, ch12, 'notes', '12.2-proc.md'), 'r', encoding='utf-8') as f:
    c_122 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.3-proc.md'), 'r', encoding='utf-8') as f:
    c_123 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.4-sysctl.md'), 'r', encoding='utf-8') as f:
    c_124 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.5-section-12-5.md'), 'r', encoding='utf-8') as f:
    c_125 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.6-sysinfo.md'), 'r', encoding='utf-8') as f:
    c_126 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.7-getnprocs-gnu.md'), 'r', encoding='utf-8') as f:
    c_127 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.8-section-12-8.md'), 'r', encoding='utf-8') as f:
    c_128 = f.read()
with open(os.path.join(BASE, ch12, 'notes', '12.9-section-12-9.md'), 'r', encoding='utf-8') as f:
    c_129 = f.read()

# Create 12.1-proc-filesystem.md (merge all /proc content)
merged_121 = c_122 + '\n\n---\n\n' + c_123 + '\n\n---\n\n' + c_124 + '\n\n---\n\n' + c_125 + '\n\n---\n\n' + c_126 + '\n\n---\n\n' + c_127 + '\n\n---\n\n' + c_128 + '\n\n---\n\n' + c_129
with open(os.path.join(BASE, ch12, 'notes', '12.1-proc-filesystem.md'), 'w', encoding='utf-8') as f:
    f.write(merged_121)
print("  CREATED: 12.1-proc-filesystem.md (merged 8 files)")

move(ch12, '12.1-uname.md', '12.2-uname.md')
for fn in ['12.2-proc.md', '12.3-proc.md', '12.4-sysctl.md', '12.5-section-12-5.md', '12.6-sysinfo.md', '12.7-getnprocs-gnu.md', '12.8-section-12-8.md', '12.9-section-12-9.md']:
    delete(ch12, fn)

create(ch12, '12.3-summary.md', SKELETON(12, '12.3', 'Summary', """本章回顾系统和进程信息。

| 来源 | API | 用途 |
|------|-----|------|
| /proc | 文件读取 | 进程/系统信息 |
| uname() | 系统调用 | 内核版本/架构 |
| sysinfo() | 系统调用 | 内存/负载/uptime"""))

create(ch12, '12.4-exercises.md', SKELETON(12, '12.4', 'Exercises', """**1.** 通过 /proc 读取进程的内存映射并解析。

**2.** 使用 uname() 打印系统信息。"""))

# ============================================================
# Ch13: File I/O Buffering (9 sections, 6 existing)
# ============================================================
print("\n=== Chapter 13 ===")
ch13 = 'chapter-13-file-io-buffering'
# 13.1 ✓, 13.2 ✓ stay
# 13.3-read-write → 13.7 (mixing stdio and syscalls)
# 13.4-posixfadvise → 13.5 (advising kernel)
# 13.5-odirect → 13.6 (direct I/O)
# 13.6-section-13-6 → 13.4 (summary of buffering) or 13.8
# Missing: 13.3 (controlling kernel buffering), 13.4 (summary of buffering),
#           13.8 (summary), 13.9 (exercises)

move(ch13, '13.3-read-write.md', '13.7-mixing-stdio-syscalls.md')
move(ch13, '13.4-posixfadvise.md', '13.5-advising-kernel.md')
move(ch13, '13.5-odirect.md', '13.6-direct-io.md')
move(ch13, '13.6-section-13-6.md', '13.8-summary.md')

create(ch13, '13.3-controlling-kernel-buffering.md', SKELETON(13, '13.3', 'Controlling Kernel Buffering of File I/O', """控制内核页缓存的刷写行为。

### API
- `fsync(fd)` — 同步文件数据和元数据
- `fdatasync(fd)` — 只同步数据（不更新 mtime 等）
- `sync()` — 刷写所有脏页
- `O_SYNC` (open flag) — 每次 write 自动同步
- `O_DSYNC` — 只同步数据
- `O_FSYNC` = `O_SYNC`

### fcntl F_SETFL
```c
int flags = fcntl(fd, F_GETFL);
flags |= O_SYNC;
fcntl(fd, F_SETFL, flags);
```

### 性能影响
| 方式 | 数据安全 | 性能 |
|------|---------|------|
| 默认（延迟写） | 低（可能丢失） | 高 |
| fdatasync | 中 | 中 |
| fsync | 高 | 低 |
| O_SYNC | 最高 | 最低 |"""))

create(ch13, '13.4-summary-buffering.md', SKELETON(13, '13.4', 'Summary of I/O Buffering', """两层缓冲总结。

### 缓冲层次
```
用户空间:  stdio 缓冲 (FILE*)
              ↓ fwrite/fread
              ↓ fflush
内核空间:  页缓存 (page cache)
              ↓ write/read
              ↓ fsync/fdatasync
磁盘:      实际存储
```

### 对比
| 层 | 管理 | 刷写方式 |
|----|------|---------|
| stdio | 用户态 | fflush() |
| page cache | 内核 | fsync() / 延迟写 |"""))

create(ch13, '13.9-exercises.md', SKELETON(13, '13.9', 'Exercises', """**1.** 解释 fsync 和 fdatasync 的区别。

**2.** 为什么混用 FILE* 和 read/write(fd) 会导致数据不一致？"""))

# ============================================================
# Ch14: File Systems (13 sections, 9 existing)
# ============================================================
print("\n=== Chapter 14 ===")
ch14 = 'chapter-14-file-systems'
# 14.1 ✓, 14.2 ✓, 14.3 ✓, 14.4 ✓, 14.5 ✓, 14.6 ✓, 14.7 ✓, 14.8 ✓, 14.9 ✓
# Need 14.10-14.13
create(ch14, '14.10-tmpfs.md', SKELETON(14, '14.10', 'A Virtual Memory File System: tmpfs', """tmpfs 是基于内存的文件系统，文件存储在页缓存中。

### 特性
- 文件存储在内存（RAM + swap），不写磁盘
- 速度快，重启后数据丢失
- 大小动态增长，受 nr_blocks 限制
- 常用于 /tmp, /dev/shm

### 挂载
```bash
mount -t tmpfs -o size=1G tmpfs /mnt/tmp
```"""))

create(ch14, '14.11-statvfs.md', SKELETON(14, '14.11', 'Obtaining Information About a File System: statvfs()', """`statvfs()` 获取文件系统信息。

```c
struct statvfs {
    unsigned long f_bsize;   // 文件系统块大小
    unsigned long f_blocks;  // 总块数
    unsigned long f_bfree;   // 空闲块数
    unsigned long f_bavail;  // 普通用户可用块数
    unsigned long f_files;   // 总 inode 数
    unsigned long f_ffree;   // 空闲 inode 数
    // ...
};
int statvfs(const char *path, struct statvfs *buf);
```"""))

create(ch14, '14.12-summary.md', SKELETON(14, '14.12', 'Summary', """本章回顾文件系统。

| 节 | 主题 |
|----|------|
| 14.1 | 设备文件 |
| 14.2 | 磁盘和分区 |
| 14.3 | 文件系统 |
| 14.4 | i-node |
| 14.5 | VFS |
| 14.6 | 日志文件系统 |
| 14.7 | 挂载点 |
| 14.8 | mount/umount |
| 14.9 | 高级挂载 |
| 14.10 | tmpfs |
| 14.11 | statvfs() |"""))

create(ch14, '14.13-exercise.md', SKELETON(14, '14.13', 'Exercise', """**1.** 编写程序使用 statvfs() 打印指定路径的文件系统使用情况。"""))

print("\n=== Batch 2 complete (Ch6-Ch14) ===")
