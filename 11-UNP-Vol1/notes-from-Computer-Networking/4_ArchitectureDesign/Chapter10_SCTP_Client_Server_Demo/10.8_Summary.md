# 10.8 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 多流实战价值

差网络下 **SCTP 多流** 消除 TCP **队头阻塞** 长尾 — 信令、多媒体调度。

### 2. 事件订阅必修课

一到多下消息带 **assoc_id、stream** 元数据 → 必须 **`SCTP_EVENTS` + sctp_data_io_event**。

### 3. 精细化状态控制

| 需求 | 手段 |
|------|------|
| 流数量 | 关联前 **`SCTP_INITMSG`** |
| 关单关联 | **`sctp_sendmsg` + SCTP_EOF** |
| 勿误用 | 一到多上 **`close` 关全部** |

---

## Ch 9 → Ch 10 实战链

```text
API（Ch9）→ 一到多回射（Ch10）→ 高级 SCTP（Ch23）
```

---

## 个人学习总结

（待填）
