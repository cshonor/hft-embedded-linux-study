# TLPI 第 57 章 — Sockets: UNIX Domain

> 对应目录：`chapter-57-sockets-unix-domain/`  
> 书名原文：**Sockets: UNIX Domain**  
> ⚠️ **本机 IPC：UDS ≫ localhost TCP（延迟）。** UNIX **DGRAM 可靠有序有边界**（≠ UDP）。路径型 bind 前 **`unlink`**（退出不自动删文件）。Linux **抽象名** `sun_path[0]='\0'`：无文件、无残留。嵌入式/HFT 守护进程常用抽象 UDS。

**优先级**：🔴（本机低延迟 IPC）  
**前置**：[Ch56 Socket 导论](../chapter-56-sockets-intro/notes.md)  
**后置**：[Ch58 TCP/IP 基础](../chapter-58-tcpip-fundamentals/notes.md)

---

## 章节目标

`sockaddr_un`；STREAM/DGRAM；权限；`socketpair`；抽象命名空间；工程选型。

---

## 57.1 `struct sockaddr_un`

```c
struct sockaddr_un {
    sa_family_t sun_family;  /* AF_UNIX */
    char sun_path[108];      /* Linux 常见长度；勿假定可移植 */
};
```

- 路径用 `snprintf` 防溢出  
- `bind` 的 `addrlen` = `offsetof(sun_path) + strlen(path) + 1`（路径型）；抽象名另算（含前导 `\0` 后的名字长度）  
- 路径型：文件系统可见 socket 文件  

---

## 57.2 STREAM

同 TCP 流程：`bind→listen→accept` / `connect`；全双工字节流。  
退出**不删** socket 文件 → 再 bind `EADDRINUSE` → bind 前 `unlink`。  
相对 pipe：**双向**。路径流式示例见 [Ch56 `us_stream`](../chapter-56-sockets-intro/code/)。

---

## 57.3 DGRAM（高频）

| | 网络 UDP | UNIX DGRAM |
|--|----------|------------|
| 可靠/有序 | 否 | **是**（内核转发） |
| 边界 | 有 | 有 |
| 满队列 | 可丢 | 发送方可**阻塞** |

两端宜各自 bind。包大小受缓冲限制。

---

## 57.4 权限

connect/发数据需对 socket 文件 **写**；父目录 **x**。  
默认创建常很宽 → 用 **umask** 收紧。简易访问控制，免自建鉴权。

---

## 57.5 `socketpair`

```c
int socketpair(int domain, int type, int protocol, int sv[2]);
```

仅 `AF_UNIX`；一对已互连、**无名**（无路径文件）。父子 IPC、双向替代 pipe。  
fork 后各方关闭不用的端（同 pipe 纪律）。

---

## 57.6 抽象命名空间（Linux）

`sun_path[0] = '\0'`，其后为名字。  

| | 路径 UDS | 抽象 |
|--|----------|------|
| 文件 | 有，须 unlink | **无** |
| 残留 | 易卡启动 | 全 close 即消 |
| 权限 | 文件 rwx | 不受目录权限 |
| 移植 | 可移植 | **仅 Linux** |

HFT/嵌入式守护进程常用抽象名，避免升级残留。

Demo：[`code/`](./code/)

---

## 选型（嵌入式 + HFT）

1. 本机组件通信 → UDS（优先于 127.0.0.1 TCP）  
2. 守护进程 → 抽象名（Linux）  
3. 父子临时双向 → `socketpair`  
4. 要消息边界且本机 → UNIX DGRAM  

---

## 思考题要点

1. 崩溃残留：启动 `unlink` 或改用抽象名。  
2. 抽象不受 umask/文件权限。  
3. fork 后关无用 fd；需半关闭用 `shutdown`。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 路径型 bind 前 unlink |
| 2 | UNIX DGRAM ≠ UDP：本地可靠 |
| 3 | connect 需写权限 + 目录 x |
| 4 | socketpair：无名互连对 |
| 5 | 抽象：`path[0]=0`，无文件 |
| 6 | 本机延迟：UDS > loopback TCP |

---

## 参考

- Kerrisk · TLPI Ch57  
- `man 7 unix` · `man 2 socketpair`
