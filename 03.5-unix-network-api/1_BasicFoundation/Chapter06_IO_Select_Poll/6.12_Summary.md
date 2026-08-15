# 6.12 小结

> 章级：[study.md](../study.md) · 下一章：[Ch 7 套接字选项](../../Chapter07_SocketOption/study.md)

---

## 核心主旨与章节提炼

### 1. I/O 复用的地位

需同时应对**多数据源**（stdin + socket、listen + 多 conn、TCP + UDP）时，**select / poll** 是最通用的**中间层**机制（本书范围内）。

### 2. 边界控制：shutdown

- **`close`**：引用计数 + 双向全关 — 粗暴  
- **`shutdown(SHUT_WR)`**：**半关闭** — 批量输入「发完仍收」的**黄金法则**

### 3. 选型

| API | 特点 |
|-----|------|
| **select** | 兼容性最强；**FD_SETSIZE**、每轮重建 fd_set |
| **poll** | 无 1024 硬顶；events/revents 分离；fd=-1 忽略 |
| **epoll/kqueue** | 高性能（本书略，见留白） |

### 4. 生产警示

**单进程多路复用 + 阻塞 I/O = DoS 脆弱** → 必须 **非阻塞 I/O**（Ch 16）。

---

## 逻辑脉络（全章链）

```text
Ch5 阻塞 str_cli
→ 6.4 select 双源监听
→ 6.5 批量 EOF 丢回射
→ 6.6 shutdown 半关闭
→ 6.7 终极客户（read/write + stdineof）
→ 6.8/6.11 单进程服务器
→ Ch16 非阻塞加固
```

---

## 易错细节速查

| 点 | 一句 |
|----|------|
| select 同步？ | 是；`read` 仍阻塞 |
| FIN | 读就绪，要 read |
| 批量 EOF | **SHUT_WR**，别 exit |
| stdio + select | 用 read/write |
| 慢客户端 | 阻塞 read 拖死全服 |

---

> 💡 **后续拓展留白**  
> - 阶段一 Ch1～6 总复盘  

---

## 个人学习总结

（待填）
