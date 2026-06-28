## 3. System V IPC 基础

> 三种经典机制共用：**键 → 标识符 → 权限**

---

### 一、三种 SysV IPC

| 机制 | 创建/获取 syscall | 用途 |
|------|-------------------|------|
| **信号量** | **`semget()`** | 同步计数 |
| **消息队列** | **`msgget()`** | 带类型消息 |
| **共享内存** | **`shmget()`** | 共享物理页 |

---

### 二、IPC 键与标识符

```
进程约定 IPC key（如 ftok 生成）
    ↓ semget / msgget / shmget
内核分配 IPC identifier（唯一句柄）
    ↓ semop / msgsnd / shmat …
实际操作资源
```

| 键 | 说明 |
|----|------|
| **约定 key** | 多应用协作 — 可能冲突，需命名规范 |
| **`IPC_PRIVATE`** | **私有** 资源 — 仅创建者及其子进程知晓 |

---

### 三、权限：`kern_ipc_perm`

每种 IPC 资源关联 **`kern_ipc_perm`**：

- **创建者 / 当前所有者**  
- **类似文件权限** 的位掩码（读/写等）  

→ 与 VFS 权限模型类比：[Ch 12](../chapter-12-VFS/)

---

← [2. 管道](./section-2-管道与FIFO.md) · 下一节 [4. 信号量](./section-4-IPC信号量.md)
