## 2. 进程凭证与安全模型

> 执行新程序前 — **谁以什么权限运行**

---

### 一、传统凭证：四种 UID / GID

每个进程维护 **real / effective / saved / filesystem** 四套 UID（GID 同理）：

| 类型 | 典型用途 |
|------|----------|
| **real** | 进程 **真实** 用户 |
| **effective** | **当前生效** 权限 — 访问检查常用 |
| **saved** | **`setuid` 切换** 时保存，便于恢复 |
| **filesystem** | **文件创建** 时的属主 |

**`setuid` 程序：** 普通用户 exec 时 **effective UID** 临时变为 root（或文件属主）— 如 `passwd`。

→ 进程模型：[Ch 3](../chapter-03-processes/)

---

### 二、能力 (Capabilities)

打破 **全能 root vs 普通用户** 二元模型：

- 特权拆成独立 **CAP_*** 标志  
- 例：`CAP_SYS_TIME`（改系统时间）、`CAP_CHOWN`（改文件属主）  

进程可 **只拥有部分** 能力 — 最小权限。

→ 用户态：[08 TLPI](../../../06-The-Linux-Programming-Interface/)

---

### 三、安全钩子 (Security Hooks)

Linux 2.6 支持 **LSM** — 如 **SELinux**：

| 机制 | 说明 |
|------|------|
| **`security_ops`** | 函数指针表 — 关键操作前 **钩子** |
| 校验点 | 改时间、**exec**、文件访问 … |

内核通用路径 + 策略模块 **强制访问控制**。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 内存布局](./section-3-地址空间与内存布局.md)
