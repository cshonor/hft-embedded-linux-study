# Ch 14 §4 Linux 蓝牙子系统 · Bluetooth

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 4. Linux 蓝牙子系统 (The Linux Bluetooth Subsystem)

**BlueZ** — Linux **官方蓝牙协议栈**；与 **共置 HFT 以太** **无关** — 知架构即可。

---

## 协议分层

```
应用 (BlueZ userspace / D-Bus)
  ↕
L2CAP / RFCOMM / BNEP / SDP …
  ↕
HCI (Host Controller Interface)
  ↕
USB/UART 蓝牙控制器固件
```

| 层 | 说明 |
|----|------|
| **HCI** | 主机 ↔ **本地控制器** 命令/事件/ACL 数据 |
| **L2CAP** | **逻辑信道** — 类似 **UDP 消息**、分段/复用 |
| **BNEP** | **以太网 over L2CAP** — **PAN 组网** |
| **RFCOMM** | **串口仿真** |

**eL2CAP (2.6.36+)** — **增强重传模式 (ERTM)**、流控 — 更可靠 L2。

---

## 内核路径

**`net/bluetooth/`** — HCI core、L2CAP、SCO…  
设备 **`hci0`** — `btmgmt`、`hcitool`。

---

← [3. Busy Poll](./section-3-忙轮询套接字与收包路径.md) · [Ch 14](../README.md) · 下一节 [5. 6LoWPAN](./section-5-IEEE802154与6LoWPAN.md)
