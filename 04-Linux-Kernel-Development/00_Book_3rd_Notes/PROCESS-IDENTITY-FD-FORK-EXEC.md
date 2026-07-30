# 身份 · 资源 · 创建逻辑（PID / FD / fork+exec / ELF）

> **主线：** 身份 → 资源 → `fork`+`execve` → 静态 ELF 如何变成可调度进程。  
> **承接：** [ELF-FORMAT-AND-PROCESS](./ELF-FORMAT-AND-PROCESS.md) · [ELF-UEFI-BOOT-CHAIN](./ELF-UEFI-BOOT-CHAIN.md) · [Ch3 进程管理](./chapter-03-process-management/) · [Ch15 地址空间](./chapter-15-process-address-space/)

---

## 0. 速记卡（先稳住再往下翻）

### 核心类比

| 调用 | 类比 |
|------|------|
| **`fork`** | **克隆出另一个「我」** — 记忆、手里的钥匙（FD）、正在跑的代码全复刻；**唯一全新的是 PID** |
| **`execve`** | **克隆体洗掉原记忆，装上新程序（ELF）** — **PID 不变**，用户态代码/数据整段替换 |

### 口诀（适合背）

- **fork：** 造新身份（新 PID），全盘复制资源，**不换程序**
- **exec：** 不换身份（PID 不变），清空用户镜像，**换新 ELF**
- **PID：** 区分进程；**FD：** 进程操作资源的钥匙

### 边界（只记这两句）

| 只做这个 | 结果 |
|----------|------|
| 只 `fork` 不 `exec` | 两个进程跑 **同一套** 代码 |
| 只 `exec` 不 `fork` | **当前进程**被新程序覆盖（自己「变成」新程序，没有另开一个身份） |
| **`fork` + `exec`** | 才是「**另起一个新身份去跑外部程序**」（Shell 启 `ls` 的模型） |

### Shell 落地：输入 `ls` 回车

```
Shell (PID=1000)
  → fork() → 子 Shell (PID=1001，代码仍是 shell，FD 已复制)
  → 子进程 execve("/bin/ls")
  → 内核读磁盘 ELF /bin/ls → 拆掉 shell 用户地址空间 → 映射 ls
  → 仍是 PID=1001，但开始跑 ls
```

---

## 主线框架

```
身份 (PID …)     资源 (FD / files_struct)
        \           /
         \         /
      task_struct（进程总档案）
              │
      fork：新身份 + 复制资源/映像（COW）
              │
      execve：身份不变 + 换 ELF 映像（重建 mm）
              │
         可调度的「跑着某个程序的进程」
```

---

## 1. 进程身份：PID

| 点 | 说明 |
|----|------|
| 是什么 | 内核为每个可调度实体（`task_struct`）分配的 **身份编号** |
| 干什么 | 调度、杀信号、`wait`、查找进程 — **靠 PID（及线程组等）定位** |
| `fork` | **新 PID** → 身份隔离；父子各有独立身份 |

**补充（同一档案里的其它「身份」字段）：**  
`task_struct` 还挂 **UID/GID**、进程组、会话 ID 等 — 权限与终端会话体系；日常口语里的「进程号」多指 **PID**。

> 现代内核还有 **PID 命名空间**（容器里看到的 PID 与宿主机不同）— 先抓「PID = 身份」即可，命名空间 Phase 后置。

---

## 2. 进程资源：FD（文件描述符）

进程是调度实体；要碰内核管理的对象（文件、管道、socket、设备），靠 **FD**。

| 点 | 说明 |
|----|------|
| 是什么 | **单个进程内部** fd 数组的 **下标**（不是全局 ID）；经 `files_struct` 指向内核对象 |
| 数量 | 一进程可持有多个 FD，指向不同对象 |
| `fork` | **复制 FD 表**；父子表项常指向 **同一底层 `file`/socket**，**引用计数 +1** |
| `execve` | **默认不关** FD；设了 **`FD_CLOEXEC`** 的才在 exec 时自动关 |

### 关键区分

| | 含义 |
|--|------|
| **PID** | 进程 **本体** 的身份（全局区分 task） |
| **FD** | 进程所持 **资源** 的钥匙（进程私有下标） |

```
PID  ──►  「我是谁」
FD   ──►  「我能开哪扇门（文件/socket/…）」
```

### `fork` 之后 FD 的坑（网络 / 高并发常踩）

```
父进程 listen_fd ──file/socket 引用计数 = 1
        │ fork
        ▼
父 listen_fd ──┐
               ├──► 同一 socket，引用计数 = 2
子 listen_fd ──┘
```

| 陷阱 | 后果 |
|------|------|
| 子进程继承了 **监听 socket**，退出时 **没 close** | 引用计数 > 0 → **端口/监听不释放**，像「谁占着端口」排查噩梦 |
| 父也一直握着子不该用的连接 fd | 连接关不干净、泄漏 |

| 解法 | |
|------|--|
| 子进程 **主动 `close` 不需要的继承 FD**（监听 fd / 其它连接） | 最常见 |
| 打开时设 **`FD_CLOEXEC`**（或 `SOCK_CLOEXEC`） | `exec` 时自动关；**纯 fork 多进程模型仍要自己 close** |
| 优先考虑 **`posix_spawn`** / 明确 fd 动作的 API | 少踩多线程下 `fork` 的坑（见文末） |

**Rust / 网络服务：** 多进程或 `Command` 前后，清点「子进程到底继承了哪些 socket」；监听 fd 只留在该听的那一侧。

---

## 3. 标准创建逻辑：`fork()` + `execve()`

### 3.1 `fork()` — 新身份 + 复制现有资源/映像

1. 拷贝（逻辑上）父进程的 `task_struct`、地址空间（**COW**）、打开的 FD、寄存器/PC 等  
2. 分配 **新 PID** → 独立子进程  
3. 父子都从 `fork` 返回后继续跑；用 **返回值** 区分（父得子 PID，子得 0）

| 此刻 | |
|------|--|
| 代码 | 父子跑 **同一套** 用户程序映像 |
| 身份 | **已隔离**（不同 PID） |
| 资源 | FD 等按规则复制 |

### 3.2 `execve()` — 身份不变，换 ELF 程序

`fork` 不会「变成新程序」；换程序靠 `execve`：

1. 传入磁盘上 **ELF 路径**  
2. `binfmt_elf` 解析头部 + **Program Header**  
3. 拆掉旧用户地址空间；按段 **mmap** 新代码/数据等  
4. 重建栈；动态程序则准备动态链接器  
5. 改入口与寄存器等上下文  
6. **PID 不变** — 档案还是那份，**跑的程序彻底换了**；`mm_struct` 常被重建

细节 → [ELF-FORMAT-AND-PROCESS](./ELF-FORMAT-AND-PROCESS.md)

---

## 4. 整条链路（合并）

```
父进程  PID=1000，持有若干 FD，跑着 shell（例）
              │ fork()
              ▼
子进程  PID=1001，COW 复制内存，复制 FD 表，仍跑 shell 代码
              │ execve("./app")
              ▼
内核读磁盘静态 ELF → 按程序头映射段 → 替换 1001 的用户地址空间
              │
              ▼
仍是 PID=1001，但开始执行 app；未设 CLOEXEC 的 FD 可仍打开
```

---

## 5. 串联问答（打通易混点）

### 静态 ELF 是什么？

磁盘上的 **二进制模板**：代码/数据/段信息。  
**没有** PID、没有已建立的用户 VA、没有打开的 FD，**不能**被调度器当任务直接跑。

### 什么时候 ELF「变成」进程？

严格说：ELF 仍是映像；**进程**是已有的 `task_struct`。  
`execve` 把该 ELF **绑定/装进** 这个 task（通常先由 `fork` 造出），使其有 PID、mm、FD 表 — 才成为「跑着该程序的可调度进程」。

### 两句刻进骨头

| 调用 | 身份 (PID) | 程序映像 | 是否新建进程 |
|------|------------|----------|--------------|
| **`fork`** | **新 PID** | 复制（COW） | **是** |
| **`execve`** | **不变** | **换成新 ELF** | **否** |

---

## 6. `task_struct` 一页档案

| 字段/子系统 | 角色 |
|-------------|------|
| **PID（及线程组）** | 档案编号 — 身份 |
| **`files_struct`** | FD 表 — 资源钥匙 |
| **`mm_struct`** | 用户虚拟地址空间 — `exec` 加载 ELF 后重建 |
| 信号、凭证 UID… | 会话与权限 |

```
task_struct
├── 身份：pid, tgid, uid/gid, 进程组/会话 …
├── 资源：files_struct → fd[] → file / socket …
├── 内存：mm_struct → VMA …（exec 后换新）
└── 调度/状态：state, on_rq …（Ch 4）
```

---

## 7. 思维导图（文本）

```
进程
├── 身份
│   ├── PID（调度/信号主键）
│   └── UID/GID · 进程组 · 会话
├── 资源
│   └── FD → files_struct（fork 复制共享底层对象；exec 默认可保留）
├── 创建
│   ├── fork  = 新 PID + 复制 mm/FD/上下文（同程序）
│   └── execve = 同 PID + 加载 ELF + 重建 mm（换程序）
└── 映像来源
    └── 磁盘 ELF（Program Header）← binfmt_elf
```

---

## 8. 延伸：`clone` / `posix_spawn`

| API | 与本文关系 |
|-----|------------|
| **`clone()`** | 更通用；**`fork` ≈ 一组固定标志的 `clone`**（Linux 视角） |
| **`posix_spawn`** | 把「创建 + 装映像 + fd/信号动作」收成一步，**规避多线程里乱 `fork` 的经典坑** |
| Rust | 高并发服务起子进程时优先查清运行时/`Command` 是否走 spawn 语义，并管好 **CLOEXEC / 显式 close** |

→ [§3.1](./chapter-03-process-management/notes/section-3.1-进程的概念.md) · [§3.2 task_struct](./chapter-03-process-management/notes/section-3.2-进程描述符与任务结构.md) · [§3.4 fork/COW](./chapter-03-process-management/notes/section-3.4-进程创建与写时拷贝.md) · [§3.5 clone/线程](./chapter-03-process-management/notes/section-3.5-Linux-的线程实现.md) · **下接调度** [Ch4 §4.1](./chapter-04-process-scheduling/notes/section-4.1-多任务与调度器演进.md)
