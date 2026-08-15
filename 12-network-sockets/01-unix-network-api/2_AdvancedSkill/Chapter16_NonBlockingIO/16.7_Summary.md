# 16.7 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 范式跨越

从顺序阻塞 → **异步事件 + 用户态缓冲状态机**（`to`/`fr`、`iptr`/`optr`）。

### 2. 非阻塞 connect

- **EINPROGRESS** + 后台握手  
- 重叠、并发、**select 超时**  
- 成功判定：**`getsockopt(SO_ERROR)`**，不能只看可写  

### 3. 非阻塞 accept

配合 **select 的 listenfd**：**必须**非阻塞，防 **RST 幽灵连接** 导致阻塞 accept 死锁。

---

## 错误码速记

| 操作 | 典型 errno |
|------|------------|
| 读/写/accept 无就绪 | **EWOULDBLOCK** / EAGAIN |
| connect 进行中 | **EINPROGRESS** |
| connect 失败（SO_ERROR） | ECONNREFUSED 等 |
| connect 超时 | ETIMEDOUT（应用层） |

---

## 与现代框架

| UNP 手写 | 现代 |
|----------|------|
| select + 非阻塞 + 缓冲 | epoll/kqueue + libuv/Netty 状态机 |

---

## 个人学习总结

（待填）
