# 9.2 接口模型

> [Ch 4 TCP API](../../1_BasicFoundation/Chapter04_BasicTCPSocket/study.md) · [Ch 10](../Chapter10_SCTP_Client_Server_Demo/study.md)

---

## 两种套接字模型

### 1. 一到一（One-to-One）

| 项 | 说明 |
|----|------|
| 类型 | **`SOCK_STREAM`** + `IPPROTO_SCTP` |
| API | 与 TCP 相同：`socket` → `bind` → `listen` → `accept` / `connect` |
| 目的 | TCP 应用**最小改动**迁移（`IPPROTO_TCP` → `IPPROTO_SCTP`） |
| 局限 | **一个 fd ↔ 一个关联** |

### 2. 一到多（One-to-Many）

| 项 | 说明 |
|----|------|
| 类型 | **`SOCK_SEQPACKET`** |
| API | 服：**无 `accept`**；客：**可不 `connect`** |
| 优势 | **一个 fd 管理多个关联**；多关联消息可交错，无需每关联一新 fd |

---

## 选型

| 场景 | 模型 |
|------|------|
| 熟悉 TCP、每连接一线程/进程 | 一到一 |
| 高并发短关联、UDP 式状态机 | 一到多 + `sctp_sendmsg`/`recvmsg` |

---

## 个人学习总结

（待填）
