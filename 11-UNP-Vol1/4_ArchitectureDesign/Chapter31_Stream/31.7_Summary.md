# 31.7 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 极致模块化

STREAMS = 用户与硬件间**可堆叠模块**的全双工消息管道；`I_PUSH`/`I_POP` 动态装配。

### 2. 控制/数据/优先级

**`putmsg`/`getmsg`** 分离控制与数据；**`putpmsg`/`getpmsg`** 处理**优先级频带**与类 OOB 语义。

### 3. TPI 与 socket 史

**TPI** 用流消息实现 `bind`/`connect` 等；SVR4 上 **socket 可能是 libc 封装**。

### 4. 现状

| 时代 | 胜者 |
|------|------|
| 标准竞争 | **Berkeley Sockets** |
| 现代 Linux/FreeBSD | 内核内置协议栈，STREAMS **基本淘汰** |
| 仍相关 | **Solaris** 维护、网络标准史 |

---

## 全书收束

```text
Ch1–8 基础 → 阶段二/三 进阶 → 架构层 SCTP/UDS/路由…
Ch30 服务器范式 → Ch31 STREAMS（历史与 SVR4 视角）
```

---

## 个人学习总结

（待填）
