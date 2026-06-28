# Ch 13 §6 内核与用户空间 API · Kernel vs Userspace

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 6. 内核层与用户空间 API 的区别 (Userspace vs Kernel-Level RDMA API)

Linux RDMA **双 API** — 语义对齐，**调用模型与权限** 不同。

---

## 对照表

| | **内核 API** | **用户空间 API** |
|---|--------------|------------------|
| 前缀 | **`ib_*`** | **`ibv_*`**（**libibverbs**） |
| 入口 | 内核模块、**SCSI/RDS** | 普通进程 **`/dev/infiniband/uverbs*`** |
| 模型 | **异步 + 回调** | **多数同步**（post → poll CQ） |
| 内存注册 | **直接 pin 用户/内核页** | 经 **uverbs ioctl** 代理 |
| CM | **`ib_cm`** | **librdmacm** |

---

## 内核异步模型

```c
/* 概念：完成以 callback / workqueue 通知 */
ib_post_send(..., completion_callback);
/* 事件：ib_handle_async_event() — QP ERR, port down, … */
```

**适用：** **IPoIB、iSER、内核 RDS** — 不能依赖用户态 poll。

---

## 用户空间 verbs

```c
ibv_post_send(qp, &wr, &bad_wr);
ibv_poll_cq(cq, n, wc);
```

**uverbs：** 每次 **create_qp/reg_mr** → **ioctl 进内核** — **启动时** 做，**热路径仅 post/poll**。

| 特权 | 说明 |
|------|------|
| **memlock ulimit** | 注册 MR 需 **锁定 RAM** |
| **Hugepages** | 大 MR 常用 **大页** 减 TLB miss |

---

## HFT 实践

| 建议 | 原因 |
|------|------|
| **用户态 verbs + busy poll CQ** | 与 **DPDK run-to-completion** 同哲学 |
| **预建 QP、预注册 MR** | 避免 tick 上 **create/destroy** |
| **单独 CM 线程** | 建链与 **数据面分离** |
| **RoCE：** **CPU affinity + NUMA 对齐 HCA** | 与 [Ch 14 NAPI/RPS](../../chapter-14-高级主题.md) 以太调优 **并列** |

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **RDMA 三协议、Linux 栈、CM/IPoIB** |
| §2 | **GUID/GID/LID、HCA/交换机** |
| §3 | **LRH/GRH/BTH、SM/SMA/CM** |
| §4 | **PD/MR/QP/CQ/SRQ/AH** |
| §5 | **WR 生命周期、Retry/RNR** |
| §6 | **`ib_` 异步 vs `ibv_` 同步** |

---

## 相关章节

- 下一章：[Ch 14 高级主题](../../chapter-14-高级主题.md) — **NAPI/softirq 🔴**
- L4 对照：[Ch 11 TCP/UDP](../../chapter-11-layer-4-protocols/)

---

← [5. WR 异常流](./section-5-工作请求处理与异常流.md) · [Ch 13](../README.md)
