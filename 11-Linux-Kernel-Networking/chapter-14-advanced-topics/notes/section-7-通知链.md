# Ch 14 §7 通知链 · Notifications Chains

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 7. 通知链 (Notifications Chains)

**Notifier chain** — 内核 **发布/订阅**：**网络设备/地址/路由** 状态变化时 **广播事件**，各子系统 **注册 callback** 响应。

---

## 网络设备事件

| 事件 | 触发时机 |
|------|----------|
| **`NETDEV_UP`** | 接口 **admin up** |
| **`NETDEV_DOWN`** | 接口 down |
| **`NETDEV_CHANGE`** | 标志/配置变 |
| **`NETDEV_CHANGEADDR`** | **MAC 变** |
| **`NETDEV_CHANGEMTU`** | **MTU 变** |
| **`NETDEV_REGISTER/UNREGISTER`** | 设备注册/注销 |

```c
register_netdevice_notifier(&nb);
/* nb.notifier_call = my_handler; */
call_netdevice_notifiers(NETDEV_UP, dev);
```

**实现：** **`notifier_call_chain()`** — 链上 **顺序调用** 各 `notifier_block`。

---

## 谁在用

| 消费者 | 反应 |
|--------|------|
| **路由/邻居** | 刷新 **FIB/ND** |
| **Bonding/Team** | 成员 **状态同步** |
| **Open vSwitch / 自定义驱动** | 重配 datapath |
| **监控模块** | 统计/日志 |

---

## HFT

**热路径不跑 notifier** — 但 **链路 flap**（`NETDEV_DOWN`）会 **invalidate 路由缓存** → **突发延迟** — DR/监控要 **订阅** 或 **外部 link 检测**。

→ 邻居刷新：[Ch 7](../../chapter-07-neighbouring-subsystem/)

---

← [6. NFC](./section-6-近场通信NFC.md) · [Ch 14](../README.md) · 下一节 [8. 杂项](./section-8-其他杂项与补充协议.md)
