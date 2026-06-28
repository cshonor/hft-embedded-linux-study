# Ch 2 §5 套接字监控 · Socket Monitoring Interface (`sock_diag`)

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. 套接字监控接口 (Socket Monitoring Interface — `sock_diag`)

**`sock_diag`** 是基于 Netlink 的 **套接字诊断** 子系统，向用户空间导出 **socket 状态、对端信息、统计** — **`ss`** 命令的核心，也是 **CRIU**（检查点/恢复）的重要数据源。

协议族：**`NETLINK_SOCK_DIAG`**。

---

## 相对 `/proc/net/*` 的补强

| | **`/proc/net/tcp` 等** | **`sock_diag` / `ss`** |
|---|------------------------|-------------------------|
| 信息深度 | 基本四元组、状态 | **inode、计时器、拥塞窗口、sk_mem、对端** |
| UNIX 域 | 对端信息 **不全** | 可获取 **peer 路径** |
| 过滤 | 文本 grep | **内核侧过滤**（`-t state` 等） |
| 性能 | 读大文件 | Netlink 请求 **按需字段** |

```bash
ss -tin   # TCP internal：rtt、cwnd、retrans
ss -x     # UNIX 套接字
ss -e     # 扩展信息（需权限）
```

---

## 典型用途

| 场景 | 说明 |
|------|------|
| **排查连接 backlog** | 行情 TCP 半连接堆积、`Recv-Q`/`Send-Q` |
| **CRIU** | 冻结/迁移容器时 **dump socket 状态** |
| **与 `lsof` 互补** | `ss` 更懂 **内核 TCP 结构** |

**HFT 运维：**

- 延迟尖刺时 **`ss -ti`** 看 **retrans/rtt** 是否飙高（仍属 **内核栈路径** 诊断）。
- **DPDK 旁路** 连接 **不在** 标准 TCP diag 里 — 别用 `ss` 查 DPDK 口上的「连接」。

---

## 消息路径（简）

```
ss (iproute2)
  → NETLINK_SOCK_DIAG 请求（指定协议 TCP/UDP/UNIX…）
  → 内核 inet_diag / unix_diag 等模块
  → 多条 SOCK_DIAG 回复（属性：state、timer、meminfo…）
```

与 §1–§3 同一套 **`nlmsghdr` + TLV** 框架；**type** 换为 **`SOCK_DIAG_BY_FAMILY`** 等。

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | Netlink **替代 ioctl**；**iproute2** 基础 |
| §2 | `socket(AF_NETLINK)` / **`netlink_kernel_create`**；**libnl** |
| §3 | **`sockaddr_nl`、`nlmsghdr`、TLV** |
| §4 | **Generic Netlink** 突破 32 族限制 |
| §5 | **`sock_diag` → `ss`、CRIU** |

---

## 相关章节

- 上一节：[4. Generic Netlink](./section-4-通用Netlink协议.md)
- 下一章：[Ch 3 ICMP](../../chapter-03-icmp/)
- 传输层 socket 内部：[Ch 11](../../chapter-11-layer-4-protocols/)
- 附录 B：[网络管理命令](../../appendix-B-网络管理.md)

---

← [4. Generic Netlink](./section-4-通用Netlink协议.md) · [Ch 2](../README.md)
