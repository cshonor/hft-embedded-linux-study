# 5.19 小结

> 章级精读：[study.md](../study.md) · 下一章：[Ch 6 I/O 多路复用](../../Chapter06_IO_Select_Poll/study.md)

---

## 核心主旨与关键论据

本章证明：**最简单的 TCP Echo** 若要真实环境常驻，必须用**信号、进程回收、错误码语义、I/O 模型**筑墙；否则僵尸、SIGPIPE、感知不到断线必然发生。

---

## 章节核心提炼

### 1. 健壮性基础设施

| 机制 | 作用 |
|------|------|
| **SIGCHLD + waitpid 循环** | 回收子进程，释放进程表 |
| **SIG_IGN SIGPIPE** | 单连接写失败不拖死全局 |
| **EINTR / SA_RESTART** | 信号与慢系统调用共存 |
| **ECONNABORTED continue** | accept 韧性 |

### 2. 异常链路与 errno

| 场景 | 典型表现 |
|------|----------|
| 网络黑洞（主机崩溃） | **ETIMEDOUT**、**EHOSTUNREACH** |
| 进程/关机 | **FIN** → 延迟感知（fgets 阻塞） |
| 重启 / 无监听 | **RST** → **ECONNRESET** |
| 写已复位连接 | **SIGPIPE** / **EPIPE** |

### 3. 数据与架构天花板

- **勿** `write` 裸 struct（5.18）  
- **阻塞模型**无法同时等 stdin 与 socket（5.5、5.12）→ **Ch 6 `select`/`poll` 革命**  

---

## 逻辑脉络：阶段一收束

```text
Ch1 时间程序 → Ch2 协议 → Ch3 地址/字节序/readn
→ Ch4 API → Ch5 健壮 Echo
→ Ch6 多路复用改造 str_cli
```

---

## 易错细节与重点结论（全章一句）

**能跑 ≠ 能上线**；上线前过 5.17 检查清单 + 计划 Ch 6 客户改造。

---

> 💡 **后续拓展留白**  
> - 阶段一 Ch1～5 总复习思维导图  

---

## 个人学习总结

（待填）
