## ① 设备类型 · Device Types

Unix/Linux 将设备分为几类（+ 扩展）：

| 类型 | 访问方式 | 特点 | 示例 |
|------|----------|------|------|
| **块设备（block）** | **块设备节点** · 常挂载 FS | **固定块大小** · **可随机寻址（seek）** | 硬盘、SSD、蓝光、闪存 |
| **字符设备（character）** | **字符设备节点** | **字节流** · **不可寻址** | 键盘、鼠标、串口 |
| **网络设备（network）** | **非设备节点** — **`socket` API** | 物理网卡 + 协议栈 | 以太网卡 |

#### 扩展类型

| 类型 | 说明 |
|------|------|
| **杂项设备（misc）** | **字符设备的简化** — 表示简单小驱动 |
| **伪设备（pseudo）** | **内核虚拟功能** — 非物理硬件 |

| 伪设备示例 | 作用 |
|------------|------|
| **`/dev/null`** | 黑洞 — 丢弃写入 |
| **`/dev/random` / `urandom`** | 随机数 |

```
「一切皆文件」：
  block/char ──► /dev/sda、/dev/ttyS0 ──► open/read/write
  network    ──► 打破该原则 ──► socket()
```

→ [Ch 1](../../chapter-01-intro/) · [Ch 13](../../chapter-13-vfs/) · [Ch 14](../../chapter-14-block-io/)

**HFT：** 行情路径走 **网卡 + socket/DPDK**；配置/调优常读 **`/sys/class/net/...`**。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 字符设备、块设备、网络设备的区别？HFT 系统中各有什么？

<details><summary>答案</summary>

字符设备：按字节流访问，无缓冲（串口/传感器/GPU）。块设备：按块随机访问，有 page cache（磁盘/SSD/NVMe）。网络设备：无 /dev 节点，通过 socket 接口（网卡）。HFT：网卡=网络设备（行情/订单），NVMe=块设备（历史数据/日志），FPGA=字符设备（自定义加速）。网络设备是最特殊的——不走 VFS。

</details>

</details>
---
