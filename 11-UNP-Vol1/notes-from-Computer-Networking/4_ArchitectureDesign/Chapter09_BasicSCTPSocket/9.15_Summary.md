# 9.15 小结

> [study.md](../study.md) · [Ch 10](../Chapter10_SCTP_Client_Server_Demo/study.md) · [Ch 23 高级 SCTP](../Chapter23_AdvancedSCTPSocket/study.md)

---

## 章节核心提炼

### 1. 范式升级

从「单字节流」→ 管理 **多 IP 路径 + 多独立消息流** 的**关联（Association）**。

### 2. 核心工具链

**`sctp_sendmsg`** / **`sctp_recvmsg`** — 流号、PPID、无序、TTL、assoc_id。

### 3. 架构灵活性

| 组件 | 作用 |
|------|------|
| **一到多** | 单 fd 多关联 |
| **`sctp_peeloff`** | 长连接剥离为独立 fd |

### 4. 状态感知

**Notifications** + `MSG_NOTIFICATION` — 关联/地址/发送失败/关闭事件。

### 5. 地址 API 成对释放

`getpaddrs`/`getladdrs` ↔ **`freepaddrs`/`freeladdrs`**

---

## API 速记

```text
多宿 bind：sctp_bindx
多宿 connect：sctp_connectx
发/收核心：sctp_sendmsg / sctp_recvmsg
 per-assoc 选项：sctp_opt_info
长连接：sctp_peeloff
无 TCP 式半关闭：shutdown 终止关联
```

---

## 个人学习总结

（待填）
