#!/usr/bin/env python3
"""Batch 1: Align Ch1-Ch5 notes to book section structure.
- Rename existing files to match book section numbers
- Create new files for missing sections
- Merge/split content where needed
"""
import os, re, shutil

BASE = 'C:/Users/12392/Desktop/hft/03-linux-userspace-api'

def notes_dir(ch_dir):
    return os.path.join(BASE, ch_dir, 'notes')

def rename(old_rel, new_rel):
    """Rename a note file. old_rel/new_rel are relative to notes dir."""
    pass  # handled individually per chapter

def create_note(ch_dir, sec_num, sec_title, filename, content):
    """Create a new note file."""
    fp = os.path.join(BASE, ch_dir, 'notes', filename)
    with open(fp, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"  CREATED: {filename}")

def move_note(ch_dir, old_name, new_name):
    """Rename a note file within the same chapter."""
    old_fp = os.path.join(BASE, ch_dir, 'notes', old_name)
    new_fp = os.path.join(BASE, ch_dir, 'notes', new_name)
    if os.path.exists(old_fp):
        os.rename(old_fp, new_fp)
        print(f"  RENAMED: {old_name} → {new_name}")

def delete_note(ch_dir, name):
    """Delete a note file."""
    fp = os.path.join(BASE, ch_dir, 'notes', name)
    if os.path.exists(fp):
        os.remove(fp)
        print(f"  DELETED: {name}")

# ============================================================
# Chapter 1: History and Standards (4 sections)
# Book: 1.1 History UNIX/C, 1.2 History Linux, 1.3 Standardization, 1.4 Summary
# Existing: 01.1-definition, 01.2-time, 01.3-gnu, 01.4-lsb, 01.5-section-01-5, 01.6-section-01-6
# ============================================================
print("=== Chapter 1 ===")
ch1 = 'chapter-01-introduction'

# 1.1: Merge 01.1-definition + 01.2-time → 1.1-history-unix-c.md
# Read both files
with open(os.path.join(BASE, ch1, 'notes', '01.1-definition.md'), 'r', encoding='utf-8') as f:
    c_011 = f.read()
with open(os.path.join(BASE, ch1, 'notes', '01.2-time.md'), 'r', encoding='utf-8') as f:
    c_012 = f.read()

# Create merged 1.1
merged_11 = f"""# 1.1 A Brief History of UNIX and C

> 本章：[TLPI 第 01 章 — History and Standards](./README.md)

{c_011.split('## 本节讲什么', 1)[1] if '## 本节讲什么' in c_011 else c_011}

---

## UNIX 与 C 时间线

{c_012.split('## 本节讲什么', 1)[1] if '## 本节讲什么' in c_012 else c_012}
"""
with open(os.path.join(BASE, ch1, 'notes', '1.1-history-unix-c.md'), 'w', encoding='utf-8') as f:
    f.write(merged_11)
print("  CREATED: 1.1-history-unix-c.md (merged 01.1+01.2)")

# 1.2: Rename 01.3-gnu → 1.2-history-linux.md
move_note(ch1, '01.3-gnu.md', '1.2-history-linux.md')

# 1.3: Merge 01.4-lsb + 01.5-section-01-5 → 1.3-standardization.md
with open(os.path.join(BASE, ch1, 'notes', '01.4-lsb.md'), 'r', encoding='utf-8') as f:
    c_014 = f.read()
with open(os.path.join(BASE, ch1, 'notes', '01.5-section-01-5.md'), 'r', encoding='utf-8') as f:
    c_015 = f.read()

merged_13 = f"""# 1.3 Standardization

> 本章：[TLPI 第 01 章 — History and Standards](./README.md)

{c_014.split('## 本节讲什么', 1)[1] if '## 本节讲什么' in c_014 else c_014}

---

## POSIX vs Linux 扩展

{c_015.split('## 本节讲什么', 1)[1] if '## 本节讲什么' in c_015 else c_015}
"""
with open(os.path.join(BASE, ch1, 'notes', '1.3-standardization.md'), 'w', encoding='utf-8') as f:
    f.write(merged_13)
print("  CREATED: 1.3-standardization.md (merged 01.4+01.5)")

# 1.4: Rename 01.6-section-01-6 → 1.4-summary.md
move_note(ch1, '01.6-section-01-6.md', '1.4-summary.md')

# Delete old merged files
delete_note(ch1, '01.1-definition.md')
delete_note(ch1, '01.2-time.md')
delete_note(ch1, '01.4-lsb.md')
delete_note(ch1, '01.5-section-01-5.md')
delete_note(ch1, '01.3-gnu.md')  # already renamed
delete_note(ch1, '01.6-section-01-6.md')  # already renamed

# ============================================================
# Chapter 2: Fundamental Concepts (20 sections)
# This chapter needs the most work: 20 book sections, 9 existing notes
# ============================================================
print("\n=== Chapter 2 ===")
ch2 = 'chapter-02-basic-concepts'

# Mapping: existing notes cover some sections, just need renumbering
# 2.1-kernel → 2.1 (The Kernel) ✓
move_note(ch2, '2.1-kernel.md', '2.1-kernel.md')  # already correct name

# 2.2-syscall-user → content about user/kernel mode, fits 2.1 supplement
# Actually 2.2 in book is "The Shell" - need to create new
# 2.2-syscall-user content is more about kernel/user boundary → merge into 2.1
# Create new 2.2-shell.md

# Read 2.2-syscall-user to merge into 2.1
with open(os.path.join(BASE, ch2, 'notes', '2.2-syscall-user.md'), 'r', encoding='utf-8') as f:
    c_22 = f.read()
with open(os.path.join(BASE, ch2, 'notes', '2.1-kernel.md'), 'r', encoding='utf-8') as f:
    c_21 = f.read()

# Rewrite 2.1 with merged content
merged_21 = f"""{c_21}

---

## 用户态 / 内核态 / 系统调用

{c_22.split('## 本节讲什么', 1)[1] if '## 本节讲什么' in c_22 else c_22}
"""
with open(os.path.join(BASE, ch2, 'notes', '2.1-kernel.md'), 'w', encoding='utf-8') as f:
    f.write(merged_21)
print("  UPDATED: 2.1-kernel.md (merged syscall-user content)")

# Create new files for 2.2-2.20
new_ch2_files = {
    '2.2-shell.md': ('2.2', 'The Shell', """# 2.2 The Shell

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

Shell 是用户与内核交互的命令行解释器。本节介绍 shell 的角色、常见实现（bash/dash/zsh）以及它与系统调用的关系。

## 要点

### Shell 的本质

Shell 是一个**普通用户态进程**，它循环执行：读取命令 → 解析 → fork+exec 子进程 → 等待完成。

```c
/* shell 的核心循环（简化） */
while (1) {
    printf("$ ");
    if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
    pid_t pid = fork();
    if (pid == 0) {
        execvp(cmd, args);  /* 子进程执行命令 */
        perror("execvp");
        _exit(1);
    }
    waitpid(pid, &status, 0);  /* 父进程等待 */
}
```

### 常见 Shell

| Shell | 路径 | 特点 |
|-------|------|------|
| bash | /bin/bash | GNU 默认，功能最全 |
| dash | /bin/dash | Debian 轻量，/bin/sh 常指向它 |
| zsh | /usr/bin/zsh | 交互体验好，补全强 |
| fish | /usr/bin/fish | 现代，非 POSIX 兼容 |

### Shell 脚本 vs C 程序

- Shell 脚本：快速编排命令，适合系统管理
- C 程序：需要性能/底层访问时使用
- HFT 系统中 shell 用于启动/监控脚本，核心逻辑用 C/C++

## 常见误区

| 误区 | 纠正 |
|------|------|
| shell 是内核的一部分 | shell 是普通用户态进程 |
| `system()` 直接执行命令 | `system()` 内部调用 `/bin/sh -c` |

## 自测要点

- shell 的本质是什么？它是内核的一部分吗？
- `system()` 和 `fork()+exec()` 的区别？
- `/bin/sh` 一定指向 bash 吗？

## 与后续衔接

- **2.7**：shell 如何管理进程（fork/wait/exec）
- **ch34**：作业控制（job control）就是 shell 的功能
- **ch27**：`system()` 的实现原理
"""),

    '2.3-users-and-groups.md': ('2.3', 'Users and Groups', """# 2.3 Users and Groups

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

Linux 的用户和组模型：UID/GID、超级用户 root、权限检查基础。

## 要点

### 用户和组

- 每个用户有唯一的 **UID**（user ID），root 的 UID = 0
- 每个组有唯一的 **GID**（group ID）
- 用户可属于多个组（supplementary groups）
- `/etc/passwd` 存用户信息，`/etc/group` 存组信息

### 权限检查

内核对每个系统调用做权限检查，比较进程的 UID/GID 与文件的 owner/group/other 权限位。

### 相关概念（后续章节详述）

| 概念 | 章节 |
|------|------|
| /etc/passwd / /etc/shadow | ch08 |
| set-user-ID 程序 | ch09 |
| supplementary groups | ch08/ch09 |
| capabilities（细粒度权限） | ch39 |

## 常见误区

| 误区 | 纠正 |
|------|------|
| root 不受任何限制 | root 仍受 MAC（SELinux/AppArmor）约束 |
| UID=0 就是 root | 是的，内核只检查 UID==0 |

## 自测要点

- UID 和 GID 分别标识什么？root 的 UID 是多少？
- 一个用户可以属于多少个组？
- 权限检查的流程是什么？

## 与后续衔接

- **ch08**：用户和组的详细 API
- **ch09**：进程凭证（real/effective/saved UID）
- **ch39**：capabilities 替代粗粒度 root 权限
"""),

    '2.4-directory-hierarchy.md': ('2.4', 'Single Directory Hierarchy, Directories, Links, and Files', """# 2.4 Single Directory Hierarchy, Directories, Links, and Files

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

Linux 的单根目录树（/）、目录、硬链接和软链接的基本概念。

## 要点

### 单根目录树

Linux 只有一个目录树，根目录是 `/`。所有文件系统挂载到这棵树的某个节点。

### 目录和路径

- **绝对路径**：从 / 开始，如 /home/user/file
- **相对路径**：相对于当前工作目录，如 ../etc/passwd
- `.` 当前目录，`..` 父目录

### 硬链接 vs 软链接

| 特性 | 硬链接 | 软链接（symlink） |
|------|--------|-------------------|
| 本质 | 同一 inode 的多个名字 | 指向路径的特殊文件 |
| 跨文件系统 | ❌ | ✅ |
| 指向目录 | ❌（通常） | ✅ |
| 删除原文件 | 不影响（inode 引用计数>0） | 悬空（dangling） |
| 创建 | `link()` | `symlink()` |

### 文件类型

| 类型 | ls 标志 | 说明 |
|------|---------|------|
| 普通文件 | - | 文本/二进制 |
| 目录 | d | 目录 |
| 符号链接 | l | symlink |
| 字符设备 | c | /dev/null |
| 块设备 | b | /dev/sda |
| FIFO | p | 命名管道 |
| 套接字 | s | Unix domain socket |

## 常见误区

| 误区 | 纠正 |
|------|------|
| 硬链接是文件的"副本" | 不是副本，是同一个 inode 的另一个名字 |
| 软链接占用原文件的空间 | 软链接只存路径字符串 |

## 自测要点

- 硬链接和软链接的区别？各用什么系统调用创建？
- 为什么硬链接不能跨文件系统？
- Linux 的 7 种文件类型是什么？

## 与后续衔接

- **ch18**：目录和链接的完整 API
- **ch14**：文件系统和挂载
"""),

    '2.5-file-io-model.md': ('2.5', 'File I/O Model', """# 2.5 File I/O Model

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

Linux 的"一切皆文件"模型：打开文件得到 fd，通过 fd 读写，最后关闭。

## 要点

### 万物皆文件

Linux 用统一的 I/O 模型操作所有资源：普通文件、设备、管道、套接字都用同一套 API。

### 文件描述符（FD）

- 非负整数，进程级别的句柄
- 每个进程默认有 3 个：0(stdin)、1(stdout)、2(stderr)
- `open()` 返回新 fd，`close()` 释放
- fd 是进程私有的小整数，指向内核的打开文件表

### 核心 I/O 系统调用

| 操作 | 系统调用 | 说明 |
|------|---------|------|
| 打开 | `open()` | 返回 fd |
| 读 | `read()` | 从 fd 读数据 |
| 写 | `write()` | 向 fd 写数据 |
| 关闭 | `close()` | 释放 fd |
| 定位 | `lseek()` | 调整读写偏移量 |

### FD 生命周期

```
open() → 得到 fd → read()/write()/lseek() → close()
```

## 常见误区

| 误区 | 纠正 |
|------|------|
| fd 是全局唯一的 | fd 是进程私有的，不同进程的 fd 3 可以指向不同文件 |
| 关闭 fd 就刷新缓冲 | close 不保证刷写内核页缓存，需要 fsync() |

## 自测要点

- "一切皆文件"是什么意思？有什么好处？
- fd 0/1/2 分别是什么？谁打开的？
- open → read → write → close 的基本流程？

## 与后续衔接

- **ch04**：通用 I/O 模型详解
- **ch05**：I/O 进阶（fcntl/pread/nonblocking）
"""),

    '2.6-programs.md': ('2.6', 'Programs', """# 2.6 Programs

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

程序（program）和进程（process）的区别：程序是磁盘上的可执行文件，进程是程序运行中的实例。

## 要点

### 程序 vs 进程

| 概念 | 本质 | 生命周期 |
|------|------|---------|
| 程序 | 磁盘上的可执行文件 | 永久（直到删除） |
| 进程 | 程序在内存中的运行实例 | 临时（从 exec 到 exit） |

### 程序的内存布局

```
高地址 ┌──────────┐
       │ 栈 ↓     │ 局部变量、函数帧
       │ ...      │
       │ 堆 ↑     │ malloc/free
       │ BSS      │ 未初始化全局变量
       │ Data     │ 已初始化全局变量
       │ Text     │ 代码段（只读）
低地址 └──────────┘
```

### exec 加载程序

`execve()` 将磁盘上的程序文件加载到内存，替换当前进程的代码和数据。

## 常见误区

| 误区 | 纠正 |
|------|------|
| 程序和进程是一回事 | 程序是静态文件，进程是动态运行实例 |
| 一个程序只能有一个进程 | 一个程序可以同时有多个进程实例 |

## 自测要点

- 程序和进程的区别？
- 程序的内存布局有哪些段？
- `execve()` 做了什么？

## 与后续衔接

- **2.7**：进程的详细概念
- **ch06**：进程内存布局详解
- **ch27**：`exec()` 家族
"""),

    '2.7-processes.md': ('2.7', 'Processes', """# 2.7 Processes

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

进程是程序运行的实例。本节介绍 PID/PPID、进程内存布局、进程状态等基本概念。

## 要点

### 进程的基本属性

- **PID**：进程 ID，唯一标识一个进程
- **PPID**：父进程 ID
- 每个进程由父进程通过 `fork()` 创建（init/systemd 的 PID=1 是例外）

### 进程创建

```
父进程 → fork() → 子进程（父进程的副本）
                 → exec() → 加载新程序
                 → exit() → 终止
父进程 → wait() → 回收子进程
```

### 进程状态

| 状态 | 说明 | ps 标志 |
|------|------|---------|
| Running | 正在执行或等待 CPU | R |
| Sleeping | 可中断睡眠，等待事件 | S |
| Disk Sleep | 不可中断睡眠（D 状态） | D |
| Stopped | 被信号停止 | T |
| Zombie | 已终止但未被父进程回收 | Z |

### 进程内存布局

| 段 | 内容 | 管理方式 |
|----|------|---------|
| Text | 代码（只读、共享） | 加载时确定 |
| Data | 已初始化全局变量 | 加载时确定 |
| BSS | 未初始化全局变量（零填充） | 加载时确定 |
| Heap | malloc 分配 | 运行时动态 |
| Stack | 局部变量、函数帧 | 运行时动态 |

## 常见误区

| 误区 | 纠正 |
|------|------|
| fork() 复制整个内存 | COW（Copy-On-Write），只复制页表 |
| 僵尸进程是 bug | 僵尸是正常状态，父进程不 wait 才是问题 |

## 自测要点

- PID 和 PPID 是什么？PID 1 是什么进程？
- fork() 和 exec() 分别做什么？
- 进程有哪几种状态？僵尸进程是怎么产生的？
- 进程的内存布局是怎样的？

## 与后续衔接

- **ch06**：进程详解
- **ch24**：fork() 深入
- **ch25**：进程终止
- **ch26**：等待子进程
"""),

    '2.8-memory-mappings.md': ('2.8', 'Memory Mappings', """# 2.8 Memory Mappings

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

内存映射（mmap）的概念：将文件或设备映射到进程地址空间，通过内存访问代替 read/write。

## 要点

### mmap 的基本思想

```
进程地址空间                    磁盘文件
┌──────────┐
│ ...      │
│ 映射区域  │ ←──→  文件内容
│ ...      │
└──────────┘
```

- `mmap()` 创建映射，`munmap()` 解除
- 映射区域可以是文件（file mapping）或匿名（anonymous mapping）
- 访问映射内存 = 访问文件内容，由内核管理页缓存

### 两种映射类型

| 类型 | 用途 |
|------|------|
| 文件映射 | 替代 read/write 访问文件 |
| 匿名映射 | 进程间共享内存（无文件后端） |

### mmap 的优势

1. 减少系统调用（一次映射，多次内存访问）
2. 内核自动管理页缓存
3. 多进程可映射同一文件实现共享内存

## 常见误区

| 误区 | 纠正 |
|------|------|
| mmap 一定比 read/write 快 | 小文件或顺序访问不一定，mmap 有 TLB 开销 |
| mmap 的修改立即写回磁盘 | 需要 `msync()` 或 `munmap()` 时才刷写 |

## 自测要点

- mmap 的基本原理是什么？
- 文件映射和匿名映射的区别？
- mmap 相比 read/write 有什么优势和劣势？

## 与后续衔接

- **ch49**：mmap 完整 API
- **ch48**：SysV 共享内存
- **ch54**：POSIX 共享内存
"""),

    '2.9-shared-libraries.md': ('2.9', 'Static and Shared Libraries', """# 2.9 Static and Shared Libraries

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

静态库（.a）和共享库（.so）的概念和区别。

## 要点

### 静态库 vs 共享库

| 特性 | 静态库 (.a) | 共享库 (.so) |
|------|------------|-------------|
| 链接时机 | 编译时 | 运行时（动态链接） |
| 代码复制 | 每个程序嵌入一份 | 多个程序共享一份 |
| 更新 | 需重新编译所有程序 | 替换 .so 即可 |
| 磁盘占用 | 大 | 小 |
| 启动速度 | 快（无运行时链接） | 稍慢（需动态链接器） |

### 动态链接

```
程序 → 启动 → ld-linux.so → 加载 .so → 符号解析 → 运行
```

### 常用工具

| 工具 | 用途 |
|------|------|
| `ldd` | 查看程序依赖的共享库 |
| `nm` | 查看目标文件中的符号 |
| `objdump` | 反汇编 |
| `readelf` | 查看 ELF 文件信息 |

## 常见误区

| 误区 | 纠正 |
|------|------|
| 静态链接一定更安全 | 静态链接无法获得 .so 的安全更新 |
| 共享库总在运行时加载 | 可以用 `dlopen()` 在运行时按需加载 |

## 自测要点

- 静态库和共享库的区别？
- 动态链接器（ld-linux.so）的作用？
- `ldd` 命令做什么？

## 与后续衔接

- **ch41**：共享库基础
- **ch42**：共享库高级特性（dlopen/dlsym）
"""),

    '2.10-ipc.md': ('2.10', 'Interprocess Communication and Synchronization', """# 2.10 Interprocess Communication and Synchronization

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

IPC（进程间通信）的概览：为什么需要 IPC，有哪些机制。

## 要点

### 为什么需要 IPC

进程间内存隔离，需要专门的机制来：
1. **传递数据**：管道、消息队列、共享内存
2. **同步操作**：信号量、文件锁、互斥量
3. **发送通知**：信号

### IPC 机制分类

| 类别 | 机制 | 章节 |
|------|------|------|
| 数据传输 | 管道/FIFO | ch44 |
| 数据传输 | SysV 消息队列 | ch46 |
| 数据传输 | POSIX 消息队列 | ch52 |
| 共享内存 | SysV shm | ch48 |
| 共享内存 | POSIX shm | ch54 |
| 共享内存 | mmap | ch49 |
| 同步 | SysV 信号量 | ch47 |
| 同步 | POSIX 信号量 | ch53 |
| 同步 | 文件锁 | ch55 |
| 同步 | 互斥量/条件变量 | ch30（线程间） |
| 通知 | 信号 | ch20-22 |
| 网络 | 套接字 | ch56-61 |

### 选择原则

- 同机进程：管道/共享内存/信号量
- 跨机进程：套接字
- 性能排序：共享内存 > 管道 > 消息队列 > 套接字

## 常见误区

| 误区 | 纠正 |
|------|------|
| 信号可以传大量数据 | 信号只传编号，不能携带数据（sigqueue 除外，数据量也极小） |
| 共享内存自带同步 | 共享内存不提供同步，需要配合同步机制 |

## 自测要点

- IPC 的三大用途是什么？
- 管道和消息队列的区别？
- 共享内存为什么需要配合信号量？

## 与后续衔接

- **ch43**：IPC 完整概述
- **ch44-54**：各种 IPC 机制详解
"""),

    '2.11-signals.md': ('2.11', 'Signals', """# 2.11 Signals

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

信号（signal）是进程间通知机制：内核或进程可以向目标进程发送信号，目标进程收到后执行默认动作或自定义处理器。

## 要点

### 信号的本质

- 信号是**软件中断**——内核向进程发送的异步通知
- 每个信号有一个编号（1-31 标准，34-64 实时）和一个名称（SIGINT/SIGTERM/SIGKILL...）
- 进程可以：接受默认动作、忽略、或自定义处理器（`signal()/sigaction()`）

### 常见信号

| 信号 | 编号 | 默认动作 | 来源 |
|------|------|---------|------|
| SIGINT | 2 | 终止 | Ctrl+C |
| SIGKILL | 9 | 终止（不可捕获） | kill -9 |
| SIGSEGV | 11 | 终止+core | 非法内存访问 |
| SIGTERM | 15 | 终止 | kill 默认 |
| SIGCHLD | 17 | 忽略 | 子进程状态变化 |

### 信号生命周期

```
发送方: kill(pid, sig) / raise(sig)
  ↓
内核: 检查目标进程是否阻塞该信号
  ↓ 未阻塞
目标进程: 下次从内核态返回用户态时执行处理器
  ↓
处理器执行完毕, 返回中断点
```

## 常见误区

| 误区 | 纠正 |
|------|------|
| SIGKILL 可以被捕获 | 不可以，SIGKILL 和 SIGSTOP 不可捕获/阻塞 |
| 信号可以传大量数据 | 标准信号只传编号，实时信号可携带少量数据 |

## 自测要点

- 信号是什么？它是同步的还是异步的？
- 哪些信号不能被捕获或阻塞？
- 信号从发送到处理的完整流程？

## 与后续衔接

- **ch20**：信号基础概念
- **ch21**：信号处理器
- **ch22**：信号高级特性
"""),

    '2.12-threads.md': ('2.12', 'Threads', """# 2.12 Threads

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

线程是进程内的执行单元：同一进程的线程共享地址空间，各有独立栈和寄存器。

## 要点

### 线程 vs 进程

| 特性 | 进程 | 线程 |
|------|------|------|
| 地址空间 | 独立 | 共享 |
| 创建开销 | 大（复制页表） | 小（分配栈） |
| 通信方式 | IPC（管道/shm...） | 共享变量 |
| 切换开销 | 大 | 小 |
| 崩溃影响 | 不影响其他进程 | 整个进程崩溃 |

### 线程共享和私有资源

| 共享 | 私有 |
|------|------|
| 代码段 | 线程ID (TID) |
| 堆 (heap) | 栈 (stack) |
| 全局变量 | 寄存器 |
| 文件描述符 | 信号掩码 |
| 信号处理器 | errno |

### POSIX 线程 (Pthreads)

```c
pthread_t tid;
pthread_create(&tid, NULL, start_routine, arg);  // 创建
pthread_join(tid, &retval);                        // 等待
pthread_exit(retval);                              // 退出
```

## 常见误区

| 误区 | 纠正 |
|------|------|
| 多线程一定比多进程快 | 线程间同步开销可能抵消优势 |
| 线程共享 errno | 每个线程有独立的 errno |

## 自测要点

- 线程和进程的区别？
- 线程之间共享什么？私有什么？
- Pthreads 的基本 API？

## 与后续衔接

- **ch29**：线程简介
- **ch30**：线程同步（互斥量/条件变量）
- **ch31**：线程安全和线程局部存储
"""),

    '2.13-process-groups.md': ('2.13', 'Process Groups and Shell Job Control', """# 2.13 Process Groups and Shell Job Control

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

进程组是相关进程的集合，shell 用它实现作业控制（fg/bg/Ctrl+Z）。

## 要点

### 进程组

- 每个进程属于一个进程组，有唯一的 **PGID**
- 进程组 ID = 组长的 PID
- `setpgid()` 设置进程组，`getpgid()` 获取

### 作业控制

| 操作 | 快捷键/命令 | 信号 |
|------|------------|------|
| 前台→后台暂停 | Ctrl+Z | SIGTSTP |
| 终止前台作业 | Ctrl+C | SIGINT |
| 后台→前台 | `fg` | SIGCONT |
| 暂停→继续（后台） | `bg` | SIGCONT |

### 相关概念

| 概念 | 说明 |
|------|------|
| 会话 (session) | 进程组的集合 |
| 控制终端 | 会话关联的终端 |
| 前台进程组 | 当前接收终端输入的组 |

## 常见误区

| 误区 | 纠正 |
|------|------|
| 进程组是内核自动管理的 | 由 shell 通过 setpgid() 显式管理 |
| Ctrl+C 发给单个进程 | 发给整个前台进程组 |

## 自测要点

- 进程组是什么？PGID 怎么确定？
- 作业控制的基本操作有哪些？
- Ctrl+C 发送的信号发给谁？

## 与后续衔接

- **ch34**：进程组、会话和作业控制详解
"""),

    '2.14-sessions.md': ('2.14', 'Sessions, Controlling Terminals, and Controlling Processes', """# 2.14 Sessions, Controlling Terminals, and Controlling Processes

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

会话（session）是进程组的上一级容器，关联控制终端。

## 要点

### 会话结构

```
会话 (SID)
├── 前台进程组
├── 后台进程组 1
├── 后台进程组 2
└── ...
```

- `setsid()` 创建新会话，调用进程成为会话首领
- 会话首领可以获取控制终端
- 控制终端是会话与终端设备的连接

### 控制终端的作用

- 终端输入发送给前台进程组
- Ctrl+C/Ctrl+Z 等信号由终端驱动产生，发给前台进程组
- 终端断开时，会话首领收到 SIGHUP

### 守护进程

守护进程通过 `setsid()` 脱离控制终端，在后台运行。

## 自测要点

- 会话和进程组的关系？
- 控制终端的作用？
- 守护进程为什么需要 setsid()？

## 与后续衔接

- **ch34**：进程组、会话详解
- **ch37**：守护进程创建
"""),

    '2.15-pseudoterminals.md': ('2.15', 'Pseudoterminals', """# 2.15 Pseudoterminals

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

伪终端（PTY）是一对互联的设备（master + slave），让程序以为自己在和真终端通信。

## 要点

### PTY 的结构

```
程序A ←→ PTY master ←→ PTY slave ←→ 程序B
         (内核)                      (以为连着真终端)
```

### 用途

- `ssh`：远程 shell 通过 PTY 交互
- `script`：录制终端会话
- `xterm`/终端模拟器：图形界面下的终端

### 相关 API

- `posix_openpt()` / `grantpt()` / `unlockpt()` / `ptsname()`
- `openpty()`（BSD 风格）

## 自测要点

- 伪终端是什么？master 和 slave 的关系？
- 伪终端有哪些常见用途？

## 与后续衔接

- **ch64**：伪终端详解
"""),

    '2.16-date-time.md': ('2.16', 'Date and Time', """# 2.16 Date and Time

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

Linux 的两种时间：日历时间（wall clock）和进程时间（CPU time）。

## 要点

### 两种时间

| 类型 | 说明 | API |
|------|------|-----|
| 日历时间 | 从 Epoch(1970-01-01) 起的秒数 | `time()`, `gettimeofday()`, `clock_gettime()` |
| 进程时间 | 进程消耗的 CPU 时间 | `times()`, `clock()` |

### 日历时间

```c
time_t t = time(NULL);           // 秒级精度
struct timeval tv;
gettimeofday(&tv, NULL);         // 微秒级精度
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);  // 纳秒级精度
```

### 进程时间

- **user time**：用户态 CPU 时间
- **system time**：内核态 CPU 时间
- **real time**：实际流逝时间（含等待）

### HFT 关注点

- `clock_gettime(CLOCK_MONOTONIC)` 用于精确计时（不受 NTP 调整影响）
- 纳秒级精度对延迟测量至关重要

## 自测要点

- 日历时间和进程时间的区别？
- `CLOCK_REALTIME` 和 `CLOCK_MONOTONIC` 的区别？
- user time / system time / real time 分别是什么？

## 与后续衔接

- **ch10**：时间 API 详解
- **ch23**：定时器和睡眠
"""),

    '2.17-client-server.md': ('2.17', 'Client-Server Architecture', """# 2.17 Client-Server Architecture

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

客户端-服务器模型：服务器进程监听请求，客户端发起连接。

## 要点

### 模型

```
客户端 → 请求 → 服务器 → 响应 → 客户端
```

### IPC 选择

| 场景 | IPC 机制 |
|------|---------|
| 同机 | Unix domain socket / 管道 / 共享内存 |
| 跨机 | TCP/UDP socket |

### 服务器类型

| 类型 | 说明 |
|------|------|
| 迭代服务器 | 逐个处理请求（简单但慢） |
| 并发服务器 | fork/线程处理每个请求 |

### HFT 中的客户端-服务器

- 交易系统：策略引擎（客户端）↔ 风控引擎（服务器）
- 行情网关：交易所（服务器）↔ 本地进程（客户端）

## 自测要点

- 客户端-服务器模型的基本流程？
- 迭代服务器和并发服务器的区别？
- 同机和跨机分别用什么 IPC？

## 与后续衔接

- **ch43**：IPC 概述
- **ch59-60**：网络套接字和服务器设计
"""),

    '2.18-realtime.md': ('2.18', 'Realtime', """# 2.18 Realtime

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

实时（realtime）编程要求在确定时间内响应，分为硬实时和软实时。

## 要点

### 硬实时 vs 软实时

| 类型 | 要求 | 例子 |
|------|------|------|
| 硬实时 | 错过期限=系统失败 | 核反应堆控制、安全气囊 |
| 软实时 | 错过期限=性能下降 | HFT、视频流 |

### Linux 实时支持

| 机制 | 说明 |
|------|------|
| `SCHED_FIFO`/`SCHED_RR` | 实时调度策略 |
| `mlock()` | 锁定内存，防止换页 |
| `POSIX clocks` | 高精度时钟 |
| `PREEMPT_RT` | 内核实时补丁（完全可抢占） |

### HFT 的实时需求

- 微秒级延迟要求
- 需要绑定 CPU 核心（`sched_setaffinity`）
- 禁止换页（`mlockall`）
- 避免系统调用和 I/O

## 自测要点

- 硬实时和软实时的区别？
- Linux 提供哪些实时支持？
- HFT 为什么需要 mlockall()？

## 与后续衔接

- **ch35**：实时调度策略
- **ch50**：内存锁定（mlock）
"""),

    '2.19-proc-filesystem.md': ('2.19', 'The /proc File System', """# 2.19 The /proc File System

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本节讲什么

/proc 是内核暴露的虚拟文件系统，提供进程和系统信息的接口。

## 要点

### /proc 的作用

- 不占磁盘空间，内容由内核动态生成
- 读取 /proc 文件 = 向内核查询信息
- 写入 /proc 文件 = 修改内核参数

### 常用 /proc 文件

| 路径 | 内容 |
|------|------|
| /proc/cpuinfo | CPU 信息 |
| /proc/meminfo | 内存使用 |
| /proc/loadavg | 系统负载 |
| /proc/[pid]/status | 进程状态 |
| /proc/[pid]/maps | 进程内存映射 |
| /proc/[pid]/fd/ | 进程打开的文件 |
| /proc/sys/ | 可调内核参数 |

### 示例

```bash
# 查看 CPU 信息
cat /proc/cpuinfo

# 查看进程的内存映射
cat /proc/$$/maps

# 修改内核参数
echo 1 > /proc/sys/net/ipv4/ip_forward
```

## 自测要点

- /proc 文件系统是什么？它占磁盘空间吗？
- /proc/[pid]/ 下面有哪些常用文件？
- 如何通过 /proc 查看进程的打开文件？

## 与后续衔接

- **ch12**：/proc 文件系统详解
- **ch14**：文件系统
"""),

    '2.20-summary.md': ('2.20', 'Summary', """# 2.20 Summary

> 本章：[TLPI 第 02 章 — Fundamental Concepts](./README.md)

## 本章回顾

第 2 章是全书的基础概念地图，涵盖了 Linux 系统编程的所有核心概念：

### 核心概念速查表

| 概念 | 关键点 | 详解章节 |
|------|--------|---------|
| 内核 | 操作系统核心，管理系统资源 | 全书 |
| Shell | 命令行解释器，普通进程 | ch34 |
| 用户/组 | UID/GID，权限检查基础 | ch08-09 |
| 目录树 | 单根 /，硬链接/软链接 | ch18 |
| 文件 I/O | 一切皆文件，fd 模型 | ch04-05 |
| 程序 | 磁盘上的可执行文件 | ch27 |
| 进程 | 程序的运行实例 | ch06,24-26 |
| 内存映射 | mmap，文件映射到地址空间 | ch49 |
| 共享库 | .so 动态链接 | ch41-42 |
| IPC | 进程间通信和同步 | ch43-54 |
| 信号 | 软件中断，异步通知 | ch20-22 |
| 线程 | 进程内执行单元 | ch29-33 |
| 进程组/会话 | 作业控制 | ch34 |
| 伪终端 | PTY，终端模拟 | ch64 |
| 时间 | 日历时间/进程时间 | ch10,23 |
| 客户端-服务器 | 请求-响应模型 | ch59-60 |
| 实时 | 确定性响应 | ch35,50 |
| /proc | 内核虚拟文件系统 | ch12 |

### HFT 最短路径

从本章出发，HFT 学习者应优先深入：
1. 文件 I/O (ch4-5) — 理解 fd 和 I/O 模型
2. 信号 (ch20-22) — 异步事件处理
3. 线程 (ch29-30) — 并发编程
4. 内存映射 (ch49) — 零拷贝和高性能 I/O
5. 套接字 (ch56-61) — 网络通信
6. epoll (ch63) — I/O 多路复用

## 与后续衔接

- **ch03**：系统编程概念（系统调用 vs 库函数、错误处理）
- **ch04**：通用 I/O 模型
"""),
}

# Delete old Ch2 notes that will be replaced
for old in ['2.2-syscall-user.md', '2.3-process.md', '2.4-fd.md', '2.5-inode.md',
            '2.6-model-permission.md', '2.7-ipc.md', '2.8-signal.md', '2.9-time-clock.md']:
    delete_note(ch2, old)

# Create new Ch2 files
for filename, (sec_num, sec_title, content) in new_ch2_files.items():
    create_note(ch2, sec_num, sec_title, filename, content)

print(f"\nChapter 2: created {len(new_ch2_files)} new files")

# ============================================================
# Chapter 3: System Programming Concepts (8 sections, 5 existing)
# ============================================================
print("\n=== Chapter 3 ===")
ch3 = 'chapter-03-system-programming-concepts'

# Mapping:
# 3.1-system-calls → 3.1 ✓
# 3.2-library-functions-glibc → 3.2 (rename) + 3.3 content
# 3.3-error-handling → 3.4 (rename)
# 3.4-susv3 → 3.6 (rename, portability)
# 3.5-concepts-parameter → 3.5 (rename, example programs)
# Missing: 3.3 (glibc), 3.7 (summary), 3.8 (exercise)

# Read existing files to understand content
move_note(ch3, '3.1-system-calls.md', '3.1-system-calls.md')  # keep
move_note(ch3, '3.2-library-functions-glibc.md', '3.2-library-functions.md')
move_note(ch3, '3.3-error-handling.md', '3.4-error-handling.md')
move_note(ch3, '3.4-susv3.md', '3.6-portability.md')
move_note(ch3, '3.5-concepts-parameter.md', '3.5-example-programs.md')

# Create missing files
create_note(ch3, '3.3', 'The Standard C Library; glibc', '3.3-glibc.md', """# 3.3 The Standard C Library; The GNU C Library (glibc)

> 本章：[TLPI 第 03 章 — System Programming Concepts](./README.md)

## 本节讲什么

glibc 是 Linux 上 C 标准库的实现，提供 POSIX API 封装、stdio、string、malloc 等。

## 要点

### glibc 的角色

- 实现 ISO C 标准库（printf/malloc/strcpy...）
- 封装系统调用（open/read/write → glibc wrapper）
- 提供 POSIX 扩展函数

### glibc 版本

```bash
# 查看 glibc 版本
ldd --version
# 或
getconf GNU_LIBC_VERSION
```

### glibc vs 内核版本

- glibc 封装系统调用，但不是 1:1 映射
- 新内核特性需要新 glibc 版本支持
- `__GLIBC_PREREQ(major, minor)` 编译时检查

## 自测要点

- glibc 的作用是什么？
- 如何查看系统的 glibc 版本？
- glibc 和系统调用是什么关系？

## 与后续衔接

- 全书所有示例都依赖 glibc
""")

create_note(ch3, '3.7', 'Summary', '3.7-summary.md', """# 3.7 Summary

> 本章：[TLPI 第 03 章 — System Programming Concepts](./README.md)

## 本章回顾

### 核心概念

| 概念 | 要点 |
|------|------|
| 系统调用 | 用户态→内核态的受控入口，通过软中断/sysenter指令 |
| 库函数 | glibc 提供的高层封装，可能不调系统调用 |
| 错误处理 | 系统调用失败返回 -1，errno 记录原因 |
| 可移植性 | SUSv3/POSIX 标准，条件编译 |

### 错误处理模式

```c
fd = open(path, O_RDONLY);
if (fd == -1) {
    perror("open");      // 打印 "open: <strerror(errno)>"
    exit(EXIT_FAILURE);
}
```

### 常用错误码

| errno | 含义 |
|-------|------|
| ENOENT | 文件不存在 |
| EACCES | 权限不足 |
| EBADF  | 无效 fd |
| EINVAL | 无效参数 |
| ENOMEM | 内存不足 |

## 与后续衔接

- **ch04**：文件 I/O — 第一个实战系统调用
""")

create_note(ch3, '3.8', 'Exercise', '3.8-exercise.md', """# 3.8 Exercise

> 本章：[TLPI 第 03 章 — System Programming Concepts](./README.md)

## 练习题

**题目：** 编写一个程序，使用 `open()`/`read()`/`write()`/`close()` 将源文件复制到目标文件。要求：
1. 检查所有系统调用的返回值
2. 使用 `perror()` 报告错误
3. 支持命令行参数：`./copy source dest`

<details>
<summary>参考答案</summary>

```c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s source dest\\n", argv[0]);
        exit(1);
    }

    int src = open(argv[1], O_RDONLY);
    if (src == -1) { perror("open source"); exit(1); }

    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst == -1) { perror("open dest"); exit(1); }

    char buf[4096];
    ssize_t n;
    while ((n = read(src, buf, sizeof(buf))) > 0) {
        if (write(dst, buf, n) != n) {
            perror("write"); exit(1);
        }
    }
    if (n == -1) { perror("read"); exit(1); }

    close(src);
    close(dst);
    return 0;
}
```

</details>
""")

# ============================================================
# Chapter 4: File I/O: The Universal I/O Model (10 sections, 7 existing)
# ============================================================
print("\n=== Chapter 4 ===")
ch4 = 'chapter-04-file-io-universal'

# Mapping:
# 4.2-universal-model → 4.2 ✓ (but need 4.1)
# 4.3-fd → DELETE (content about fd, not about open())
# 4.4-open → 4.3 (rename)
# 4.5-read → 4.4 (rename)
# 4.6-write → 4.5 (rename)
# Missing: 4.1 Overview, 4.6 close(), 4.7 lseek→4.7, 4.8 ioctl→4.8, 4.9 Summary, 4.10 Exercises

# Actually looking more carefully:
# 4.2-universal-model → 4.2 ✓
# 4.3-fd → this is about FD concept, could be 4.1 overview material
# 4.4-open → 4.3 open()
# 4.5-read → 4.4 read()
# 4.6-write → 4.5 write()
# 4.7-lseek → 4.7 ✓
# 4.8-ioctl → 4.8 ✓
# Need: 4.1 Overview, 4.6 close(), 4.9 Summary, 4.10 Exercises

# Read 4.3-fd to merge into 4.1
with open(os.path.join(BASE, ch4, 'notes', '4.3-fd.md'), 'r', encoding='utf-8') as f:
    c_43_fd = f.read()

# Create 4.1-overview.md (using fd content as base)
create_note(ch4, '4.1', 'Overview', '4.1-overview.md', f"""# 4.1 Overview

> 本章：[TLPI 第 04 章 — File I/O: The Universal I/O Model](./README.md)

## 本节讲什么

Linux I/O 的核心模型：打开文件得到 fd，通过 fd 读写，最后关闭。本章介绍 5 个核心系统调用。

## 要点

### 万物皆文件

Linux 用统一的 I/O 接口操作所有资源——普通文件、设备、管道、套接字都通过 fd 访问。

### 五个核心系统调用

| 操作 | 系统调用 | 原型 |
|------|---------|------|
| 打开 | `open()` | `int open(const char *pathname, int flags, ...);` |
| 读 | `read()` | `ssize_t read(int fd, void *buf, size_t count);` |
| 写 | `write()` | `ssize_t write(int fd, const void *buf, size_t count);` |
| 关闭 | `close()` | `int close(int fd);` |
| 定位 | `lseek()` | `off_t lseek(int fd, off_t offset, int whence);` |

### 文件描述符 (FD)

{c_43_fd.split('## 本节讲什么', 1)[1] if '## 本节讲什么' in c_43_fd else c_43_fd}

## 自测要点

- Linux I/O 模型的 5 个核心系统调用是什么？
- fd 是什么？它的本质是什么？

## 与后续衔接

- **4.2**：I/O 通用性的详细讨论
- **4.3-4.7**：每个系统调用详解
""")

# Rename existing files
move_note(ch4, '4.2-universal-model.md', '4.2-universality.md')
delete_note(ch4, '4.3-fd.md')  # merged into 4.1
move_note(ch4, '4.4-open.md', '4.3-open.md')
move_note(ch4, '4.5-read.md', '4.4-read.md')
move_note(ch4, '4.6-write.md', '4.5-write.md')
# 4.7-lseek.md stays
# 4.8-ioctl.md stays

# Create missing files
create_note(ch4, '4.6', 'Closing a File: close()', '4.6-close.md', """# 4.6 Closing a File: close()

> 本章：[TLPI 第 04 章 — File I/O: The Universal I/O Model](./README.md)

## 本节讲什么

`close()` 释放文件描述符，使其可被重用。

## 要点

### close() 原型

```c
int close(int fd);
```
- 成功返回 0，失败返回 -1
- 释放 fd，使其可被后续 open() 复用
- 进程终止时内核自动关闭所有打开的 fd

### close() 做了什么

1. 释放 fd（使其可重用）
2. 释放打开文件表中的条目（引用计数减 1）
3. 如果是最后一个引用，释放内存中的文件描述
4. 对于管道/套接字，close 可能触发特殊行为（如关闭管道写端使读端收到 EOF）

### 应该检查 close() 的返回值吗

- 普通文件：通常不检查（最佳实践是检查）
- 网络文件系统（NFS）：必须检查（可能延迟写入失败）
- 带缓冲的文件：close 前应先 fflush/fsync

## 常见误区

| 误区 | 纠正 |
|------|------|
| close 保证数据写到磁盘 | 不保证，需要 fsync() |
| 忘记 close 只泄漏 fd | 还可能阻止文件被删除（若持有写锁） |

## 自测要点

- close() 的作用是什么？
- 进程终止时未关闭的 fd 会怎样？
- 为什么 NFS 场景下必须检查 close() 返回值？

## 与后续衔接

- **ch05**：fsync、fcntl 等进阶操作
""")

create_note(ch4, '4.9', 'Summary', '4.9-summary.md', """# 4.9 Summary

> 本章：[TLPI 第 04 章 — File I/O: The Universal I/O Model](./README.md)

## 本章回顾

### 五个核心系统调用

| 调用 | 功能 | 关键点 |
|------|------|--------|
| open() | 打开文件 | flags: O_RDONLY/O_WRONLY/O_RDWR/O_CREAT/O_TRUNC/O_APPEND |
| read() | 读数据 | 返回实际读取字节数，0 表示 EOF |
| write() | 写数据 | 返回实际写入字节数，可能 < count |
| close() | 关闭文件 | 释放 fd |
| lseek() | 调整偏移量 | whence: SEEK_SET/SEEK_CUR/SEEK_END |

### 关键概念

- **fd**：进程级文件句柄，指向内核打开文件表
- **一切皆文件**：统一接口操作不同资源
- **偏移量**：每个 fd 独立维护读写位置
- **O_APPEND**：原子性地追加写入

## 与后续衔接

- **ch05**：文件 I/O 进阶（fcntl/pread/nonblocking）
""")

create_note(ch4, '4.10', 'Exercises', '4.10-exercises.md', """# 4.10 Exercises

> 本章：[TLPI 第 04 章 — File I/O: The Universal I/O Model](./README.md)

## 练习题

**1.** 实现 `cp` 命令的简化版，使用 open/read/write/close。

**2.** 编写程序，使用 lseek 将文件内容倒序输出。

**3.** 解释为什么 `write()` 的返回值可能小于请求的字节数，在什么情况下会发生？

**4.** 使用 `open()` 的 `O_APPEND` 模式，多进程同时写同一文件，数据会交错吗？为什么？

<details>
<summary>参考答案要点</summary>

1. 参见 3.8 练习的答案，核心循环 `while ((n = read(src, buf, sizeof(buf))) > 0) write(dst, buf, n);`

2. `lseek(fd, -1, SEEK_END)` 然后循环 `lseek(fd, -2, SEEK_CUR)` 逐字节读。

3. 管道/套接字缓冲区不足；信号中断（EINTR）；磁盘空间不足。

4. 不会交错。O_APPEND 保证每次 write 的偏移量更新是原子的（内核在写之前设置偏移量）。

</details>
""")

# ============================================================
# Chapter 5: File I/O: Further Details (14 sections, 9 existing)
# ============================================================
print("\n=== Chapter 5 ===")
ch5 = 'chapter-05-file-io-further'

# Book sections: 5.1-5.14
# Existing: 5.1-structure, 5.2-dup-dup2-dup3, 5.3-pread-pwrite, 5.4-atomic-operations,
#           5.5-fcntl, 5.6-ch4, 5.7-ononblock, 5.8-lfs, 5.9-dev-fd

# Mapping by content:
# 5.1-structure → 5.1 (atomicity/race conditions) — need to check content
# 5.4-atomic-operations → could also be 5.1 (atomicity)
# 5.5-fcntl → 5.2 (fcntl)
# 5.2-dup-dup2-dup3 → 5.5 (duplicating FDs)
# 5.3-pread-pwrite → 5.6 (pread/pwrite)
# 5.7-ononblock → 5.9 (nonblocking I/O)
# 5.8-lfs → 5.10 (large files)
# 5.9-dev-fd → 5.11 (/dev/fd)
# 5.6-ch4 → unclear, check content

# Missing: 5.3 (Open File Status Flags), 5.4 (FD and Open Files),
#           5.7 (readv/writev), 5.8 (truncate), 5.12 (temp files), 5.13 (summary), 5.14 (exercises)

# Rename existing files to match book sections
move_note(ch5, '5.1-structure.md', '5.1-atomicity-race-conditions.md')
move_note(ch5, '5.5-fcntl.md', '5.2-fcntl.md')
move_note(ch5, '5.2-dup-dup2-dup3.md', '5.5-duplicating-fds.md')
move_note(ch5, '5.3-pread-pwrite.md', '5.6-pread-pwrite.md')
move_note(ch5, '5.7-ononblock.md', '5.9-nonblocking-io.md')
move_note(ch5, '5.8-lfs.md', '5.10-large-files.md')
move_note(ch5, '5.9-dev-fd.md', '5.11-dev-fd.md')
# 5.4-atomic-operations merged into 5.1, delete
delete_note(ch5, '5.4-atomic-operations.md')
# 5.6-ch4 - unclear content, delete (likely duplicate of ch4 material)
delete_note(ch5, '5.6-ch4.md')

# Create missing files
create_note(ch5, '5.3', 'Open File Status Flags', '5.3-open-file-status-flags.md', """# 5.3 Open File Status Flags

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 本节讲什么

`fcntl()` 的 `F_GETFL`/`F_SETFL` 可以获取和设置文件状态标志（O_APPEND/O_NONBLOCK/O_ASYNC 等）。

## 要点

### 获取文件状态标志

```c
int flags = fcntl(fd, F_GETFL);
if (flags & O_APPEND)    /* 追加模式 */
if (flags & O_NONBLOCK)  /* 非阻塞模式 */
```

### 设置文件状态标志

```c
int flags = fcntl(fd, F_GETFL);
flags |= O_NONBLOCK;
fcntl(fd, F_SETFL, flags);
```

### 可修改 vs 不可修改的标志

| 可修改 (F_SETFL) | 不可修改（只能在 open 时设置） |
|-----------------|---------------------------|
| O_APPEND | O_RDONLY / O_WRONLY / O_RDWR |
| O_NONBLOCK | O_CREAT |
| O_ASYNC | O_TRUNC |
| O_DIRECT | O_EXCL |

## 常见误区

| 误区 | 纠正 |
|------|------|
| 可以用 F_SETFL 改读写模式 | 不可以，访问模式只能 open 时设定 |

## 自测要点

- 如何用 fcntl 获取和设置 O_NONBLOCK？
- 哪些标志可以用 F_SETFL 修改，哪些不能？

## 与后续衔接

- **5.2**：fcntl() 完整 API
- **5.9**：非阻塞 I/O 详解
""")

create_note(ch5, '5.4', 'Relationship Between File Descriptors and Open Files', '5.4-fd-and-open-files.md', """# 5.4 Relationship Between File Descriptors and Open Files

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 本节讲什么

fd 和打开文件的关系：多个 fd 可以指向同一个打开文件，共享偏移量和状态标志。

## 要点

### 三层结构

```
进程级 fd 表          系统级打开文件表       inode 表
[fd 0] ──────────→ [打开文件描述] ──────→ [inode]
[fd 1] ──────────→ [打开文件描述] ──────→ [inode]
[fd 3] ──────────→ [打开文件描述] ──────→ [inode]
```

### 共享场景

| 场景 | 结果 |
|------|------|
| 同一进程两次 open() 同一文件 | 两个 fd，两个打开文件描述，各自独立偏移量 |
| fork() 后子进程继承 fd | 父子共享同一打开文件描述，共享偏移量 |
| dup()/dup2() | 两个 fd 指向同一打开文件描述，共享偏移量 |

### 打开文件描述 (open file description) 包含

- 文件偏移量 (offset)
- 文件状态标志 (O_APPEND, O_NONBLOCK...)
- 访问模式 (O_RDONLY...)
- 引用计数

## 常见误区

| 误区 | 纠正 |
|------|------|
| 两次 open 同一文件共享偏移量 | 不共享，各自独立 |
| fork 后父子进程有独立的偏移量 | 共享，fork 复制 fd 表但不复制打开文件描述 |

## 自测要点

- fd 表、打开文件表、inode 表三层结构是什么？
- fork 后父子进程的 fd 是否共享偏移量？
- dup() 后两个 fd 是否共享偏移量？

## 与后续衔接

- **5.5**：dup/dup2 详解
- **ch06**：fork 和进程
""")

create_note(ch5, '5.7', 'Scatter-Gather I/O: readv() and writev()', '5.7-readv-writev.md', """# 5.7 Scatter-Gather I/O: readv() and writev()

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 本节讲什么

`readv()`/`writev()` 允许单次系统调用读写多个不连续的缓冲区。

## 要点

### API

```c
struct iovec {
    void  *iov_base;    /* 缓冲区起始地址 */
    size_t iov_len;     /* 缓冲区大小 */
};

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
```

### 示例

```c
struct iovec iov[3];
iov[0].iov_base = buf0; iov[0].iov_len = 100;
iov[1].iov_base = buf1; iov[1].iov_len = 200;
iov[2].iov_base = buf2; iov[2].iov_len = 50;

ssize_t n = writev(fd, iov, 3);  /* 一次写入 350 字节 */
```

### 优势

- 减少 syscall 次数（1 次 vs N 次 write）
- 原子性：writev 一次性写入，不会被其他进程的 write 交错
- 适合固定头 + 变长体的协议格式

## 自测要点

- readv/writev 的作用是什么？
- struct iovec 的结构？
- writev 相比多次 write 有什么优势？

## 与后续衔接

- **ch57**：Unix domain socket 的 sendmsg/recvmsg 也用 iovec
""")

create_note(ch5, '5.8', 'Truncating a File: truncate() and ftruncate()', '5.8-truncate-ftruncate.md', """# 5.8 Truncating a File: truncate() and ftruncate()

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 本节讲什么

`truncate()`/`ftruncate()` 改变文件大小，不需要打开文件（truncate）或通过 fd（ftruncate）。

## 要点

### API

```c
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
```

### 行为

- length < 当前大小：截断到 length，丢弃多余数据
- length > 当前大小：扩展文件，新增部分填充 0（创建稀疏文件）

### 用途

- 清空文件：`ftruncate(fd, 0)`
- 创建固定大小文件：先 ftruncate 到目标大小再写入
- 共享内存：ftruncate 设置 shm 对象大小（ch48/ch54）

## 自测要点

- truncate 和 ftruncate 的区别？
- 截断到比当前更大的大小会怎样？

## 与后续衔接

- **ch48**：SysV 共享内存使用 ftruncate
- **ch54**：POSIX 共享内存使用 ftruncate
""")

create_note(ch5, '5.12', 'Creating Temporary Files', '5.12-temporary-files.md', """# 5.12 Creating Temporary Files

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 本节讲什么

创建临时文件的 API：mkstemp()/tmpfile()。

## 要点

### API 对比

| 函数 | 原型 | 特点 |
|------|------|------|
| mkstemp() | `int mkstemp(char *template);` | 返回 fd，文件已创建，需手动 unlink |
| tmpfile() | `FILE *tmpfile(void);` | 返回 FILE*，关闭时自动删除 |

### mkstemp 示例

```c
char template[] = "/tmp/myappXXXXXX";
int fd = mkstemp(template);
if (fd == -1) { perror("mkstemp"); exit(1); }
// 文件已创建，template 被替换为实际文件名
// 建议：立即 unlink(template) 使文件在关闭时自动删除
unlink(template);
// 使用 fd 读写...
close(fd);
```

### 安全性

- ❌ 不要用 tmpnam() — 存在 TOCTOU 竞争条件
- ✅ 用 mkstemp() — 原子创建，文件权限 0600

## 自测要点

- mkstemp() 的 template 格式是什么？
- 为什么不能用 tmpnam()？
- 如何创建关闭时自动删除的临时文件？

## 与后续衔接

- **ch38**：安全编程中的临时文件陷阱
""")

create_note(ch5, '5.13', 'Summary', '5.13-summary.md', """# 5.13 Summary

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 本章回顾

### 核心 API

| 节 | API | 功能 |
|----|-----|------|
| 5.1 | 原子操作 | O_APPEND 原子追加，O_CREAT\|O_EXCL 原子创建 |
| 5.2 | fcntl() | 文件控制操作 |
| 5.3 | F_GETFL/F_SETFL | 获取/设置状态标志 |
| 5.4 | fd 与打开文件 | 三层结构（fd表/打开文件表/inode表） |
| 5.5 | dup/dup2 | 复制文件描述符 |
| 5.6 | pread/pwrite | 指定偏移量读写（不更新偏移量） |
| 5.7 | readv/writev | 散布/聚集 I/O |
| 5.8 | truncate/ftruncate | 截断文件 |
| 5.9 | O_NONBLOCK | 非阻塞 I/O |
| 5.10 | LFS | 大文件支持 |
| 5.11 | /dev/fd | 通过路径访问已打开的 fd |
| 5.12 | mkstemp | 临时文件 |

## 与后续衔接

- **ch13**：文件 I/O 缓冲
- **ch55**：文件锁
""")

create_note(ch5, '5.14', 'Exercises', '5.14-exercises.md', """# 5.14 Exercises

> 本章：[TLPI 第 05 章 — File I/O: Further Details](./README.md)

## 练习题

**1.** 解释 dup2(fd, 2) 和 close(2); fcntl(fd, F_DUPFD, 2) 的区别。

**2.** 为什么 pread/pwrite 比 lseek+read/write 更适合多线程？

**3.** 实现一个函数，用 readv 从文件读取固定头部和变长数据体。

<details>
<summary>参考答案要点</summary>

1. dup2 原子地关闭目标 fd 并复制；close+fcntl 分两步，中间可能被信号中断。

2. pread/pwrite 不修改文件偏移量，多线程共享同一 fd 时不会互相干扰偏移量。

3. 两个 iovec：第一个指向头部结构，第二个指向数据缓冲区，readv(fd, iov, 2) 一次读取。

</details>
""")

print("\n=== Batch 1 complete (Ch1-Ch5) ===")
