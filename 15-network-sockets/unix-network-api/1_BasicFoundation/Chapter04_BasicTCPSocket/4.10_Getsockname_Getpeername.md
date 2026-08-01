# 4.10 getsockname 和 getpeername 函数

---

## 核心主旨与关键论据

运行时获取连接**本地/远端**协议地址。

| 函数 | 返回 |
|------|------|
| **`getsockname`** | **本地** IP + 端口 |
| **`getpeername`** | **远端** IP + 端口 |

---

## 典型应用场景

| # | 场景 |
|---|------|
| 1 | `bind` 时端口为 **0** → 用 `getsockname` 查内核分配的**临时端口** |
| 2 | `bind` **`INADDR_ANY`** → 对 **accept 返回的 connfd** 调 `getsockname` 知连接落在**哪块网卡** |
| 3 | **inetd**：父 `accept`+`fork`+`exec` 后，子程序无 `cliaddr` → 启动后 **`getpeername`** 获客户身份 |

---

## 易错细节

- 对 **listenfd** 调 `getpeername` 无意义（无对端）  
- 参数仍是 **值—结果** `socklen_t*`（Ch 3.3）

---

## 个人学习总结

（待填）
