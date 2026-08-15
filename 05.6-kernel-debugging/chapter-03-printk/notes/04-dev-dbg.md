# 3.4 dev_dbg 与设备相关调试

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

`dev_dbg()` 是驱动开发中最常用的调试宏——自动添加设备名前缀，受 dyndbg 控制，适合多设备场景。

## dev_dbg 系列

### 基本用法

```c
// dev_dbg 自动添加设备名前缀
dev_dbg(dev, "config: 0x%x\n", config);
// 输出: my_device: config: 0x1234

// 其他级别
dev_emerg(dev, "hardware failure\n");
dev_alert(dev, "critical: temperature=%d\n", temp);
dev_crit(dev, "PCIe link down\n");
dev_err(dev, "failed to read register: %d\n", ret);
dev_warn(dev, "fallback to default config\n");
dev_notice(dev, "firmware version %d.%d\n", major, minor);
dev_info(dev, "driver version 1.0 loaded\n");
dev_dbg(dev, "register read: 0x%x\n", reg_val);

// 设备相关速率限制
dev_err_ratelimited(dev, "interrupt timeout (count=%d)\n", count);
dev_warn_ratelimited(dev, "rx queue full\n");
dev_dbg_ratelimited(dev, "retry %d\n", retry_count);
```

### 设备名格式

```c
// dev_name() 返回设备名:
// PCI 设备: "0000:01:00.0"
// USB 设备: "1-2:1.0"
// 平台设备: "fe200000.serial"
// 设备树: "serial@7e201000"

// 输出示例:
// 0000:01:00.0: config: 0x1234        (PCI)
// fe200000.serial: register read: 0x5  (平台)
// 1-2:1.0: interface up               (USB)
```

## netdev_dbg / netif_* 系列

### 网络设备专用宏

```c
// netdev_dbg: 网络设备调试
struct net_device *netdev = pci_get_drvdata(pdev);
netdev_dbg(netdev, "tx: skb=%p len=%u\n", skb, skb->len);
// 输出: eth0: tx: skb=ffff000012345678 len=64

// 其他级别
netdev_err(netdev, "link down\n");
netdev_warn(netdev, "tx timeout\n");
netdev_info(netdev, "link up: %dMbps %s-duplex\n", speed, duplex);

// netif_* 系列: 带消息分类
netif_err(netdev, probe, dev, "failed to initialize\n");
netif_warn(netdev, link, dev, "link timeout\n");
netif_dbg(netdev, drv, dev, "resume complete\n");
netif_info(netdev, ifup, dev, "interface up\n");
```

### 消息分类 (msglevel)

| 分类 | 含义 | 启用方法 |
|------|------|---------|
| `drv` | 驱动通用 | `ethtool -s eth0 msglevel drv on` |
| `probe` | 探测/初始化 | `ethtool -s eth0 msglevel probe on` |
| `link` | 链路状态 | `ethtool -s eth0 msglevel link on` |
| `ifup` | 接口启用 | `ethtool -s eth0 msglevel ifup on` |
| `ifdown` | 接口关闭 | `ethtool -s eth0 msglevel ifdown on` |
| `tx_err` | 发送错误 | `ethtool -s eth0 msglevel tx_err on` |
| `rx_err` | 接收错误 | `ethtool -s eth0 msglevel rx_err on` |
| `tx_done` | 发送完成 | `ethtool -s eth0 msglevel tx_done on` |

```bash
# 查看当前 msglevel
ethtool --show-msglvl eth0

# 设置 msglevel
ethtool --set-msglvl eth0 \
    drv on \
    link on \
    tx_err on \
    rx_err on
```

## dev_dbg vs pr_debug vs printk

| 特性 | pr_debug | dev_dbg | printk(KERN_DEBUG) |
|------|----------|---------|-------------------|
| 设备名前缀 | ❌ | ✅ 自动 | ❌ |
| 上下文要求 | 任何 | 需 `struct device *` | 任何 |
| dyndbg 支持 | ✅ | ✅ | ❌ |
| 消息分类 | ❌ | ❌ | ❌ |
| 运行时开销（关闭时） | ~1ns | ~1ns | 始终执行 |
| 适用 | 通用代码 | 驱动代码 | 旧代码 |

## netif_dbg vs dev_dbg

```c
// dev_dbg: 通用设备调试
dev_dbg(&pdev->dev, "config: 0x%x\n", config);

// netdev_dbg: 网络设备调试（包含接口名）
netdev_dbg(netdev, "tx: skb=%p\n", skb);

// netif_dbg: 带分类的网络调试
netif_dbg(netdev, drv, netdev, "resume complete\n");
// 只在 msglevel 中 drv 分类启用时输出
// 比 dyndbg 更细粒度（按功能分类）
```

## 多设备实例调试

```c
// HFT 场景: 多块相同网卡
// dev_dbg 自动添加设备名区分
static irqreturn_t hft_irq_handler(int irq, void *dev_id)
{
    struct hft_dev *dev = dev_id;

    // 输出: 0000:01:00.0: irq handled, status=0x1234
    dev_dbg(dev->dev, "irq handled, status=0x%x\n", dev->status);

    // 输出: 0000:02:00.0: irq handled, status=0x5678
    // 另一块网卡的日志自动区分

    return IRQ_HANDLED;
}
```

## HFT 关联

HFT 自定义 PCIe 设备驱动应统一使用 `dev_dbg()`：

1. **自动设备名**：多设备实例日志自动区分
2. **dyndbg 控制**：生产环境按需开关
3. **速率限制**：`dev_err_ratelimited()` 防止日志风暴
4. **网络设备**：`netdev_dbg()` + `netif_dbg()` 带分类

```c
// HFT 驱动调试规范
struct hft_dev {
    struct pci_dev *pdev;
    struct net_device *netdev;
    // ...
};

static int hft_init_hw(struct hft_dev *dev)
{
    struct device *d = &dev->pdev->dev;

    dev_dbg(d, "initializing hardware\n");
    dev_dbg(d, "bar0=%pR\n", &dev->pdev->resource[0]);

    // 错误用 dev_err
    int ret = hft_reset(dev);
    if (ret) {
        dev_err(d, "reset failed: %d\n", ret);
        return ret;
    }

    // 网络相关用 netdev_dbg
    netdev_dbg(dev->netdev, "MAC: %pM\n", dev->netdev->dev_addr);

    // 中断处理用速率限制
    // 在 IRQ handler 中:
    // dev_err_ratelimited(d, "irq error: status=0x%x\n", status);

    return 0;
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** dev_dbg 如何获取设备名？输出格式是什么？

> `dev_dbg()` 从传入的 `struct device *` 中提取 `dev_name(dev)`（通常是总线地址或设备树节点名）。输出格式为 `dev_name: message`。例如 PCI 设备 `0000:01:00.0: config: 0x1234`。

**Q2:** `netif_dbg(tp, drv, dev, ...)` 中的第二个参数是什么？

> 是消息分类 (msglevel)。网络子系统支持按分类过滤消息：drv（驱动）、probe（探测）、link（链路）、ifdown（接口关闭）等。通过 `ethtool -s eth0 msglevel drv on` 动态控制。比 dyndbg 更细粒度但仅适用于网络设备。

**Q3:** dev_dbg() 在未启用 Dynamic Debug 时是否有开销？

> 几乎无开销——编译为对 `_ddebug` 描述符 flags 的检查（约 1ns）。如果 flags 未设置 _DPRINTK_FLAGS_PRINT 则直接返回。实际格式化和输出只在启用时发生。这是 dev_dbg 相比 printk(KERN_DEBUG) 的优势：未启用时几乎零开销。

**Q4:** 为什么多设备场景应该用 dev_dbg 而不是 pr_debug？

> dev_dbg 自动添加设备名前缀（如 "0000:01:00.0:"），日志中可以立即区分是哪块设备。pr_debug 没有设备名，多设备场景下日志混杂无法区分。HFT 系统通常有多块相同网卡，dev_dbg 的设备名前缀至关重要。

**Q5:** netif_dbg 和 dev_dbg 在网络驱动中如何选择？

> 网络协议相关（链路状态、收发包、接口启停）用 netif_dbg + 分类（如 `netif_dbg(netdev, link, ...)`）。硬件相关（寄存器、DMA、中断）用 dev_dbg。netif_dbg 支持按功能分类过滤（ethtool msglevel），dev_dbg 支持 dyndbg 按文件/函数过滤。

</details>

## 交叉引用

- [05.6 ch03 printk 基础](chapter-03-printk/notes/01-printk-basics-loglevel.md)
- [05.6 ch03 dynamic debug](chapter-03-printk/notes/03-dynamic-debug.md)
- [05.6 ch03 trace_printk](chapter-03-printk/notes/05-ftrace-printk.md)
