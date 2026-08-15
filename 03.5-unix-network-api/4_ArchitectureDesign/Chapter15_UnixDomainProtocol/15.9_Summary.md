# 15.9 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 同机 IPC 最优解

明确同机部署 → 优先 **`AF_LOCAL`**，弃 TCP `127.0.0.1` — 性能与延迟明显改善。

### 2. 文件系统清理

`bind` 创建真实 socket 文件 → 启动 **`unlink(path)`**、退出时清理 — 守护进程基本素养。

### 3. 特权与架构

**`sendmsg` + `SCM_RIGHTS`** 描述符传递 — Master/Worker、多进程分担连接的内功。

### 4. 流 vs 报

| 类型 | 要点 |
|------|------|
| `SOCK_STREAM` | 类 TCP，无网络丢包语义 |
| `SOCK_DGRAM` | 类 UDP 但有边界；**可靠**；客户须 **bind** |

---

## 学习路径

```text
Ch14 sendmsg/辅助数据 → Ch15 UDS + fd 传递 → Ch13 守护进程实践
```

---

## 个人学习总结

（待填）
