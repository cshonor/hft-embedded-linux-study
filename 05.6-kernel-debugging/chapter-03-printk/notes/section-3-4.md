# 3.4 dev_dbg 与设备相关调试

> 🔴 精读

## 本节要点

### dev_dbg 系列

```c
// dev_dbg 自动添加设备名前缀
dev_dbg(dev, "config: 0x%x\n", config);
// 输出: my_device: config: 0x1234

// 其他级别
dev_err(dev, "failed to read register: %d\n", ret);
dev_warn(dev, "fallback to default config\n");
dev_info(dev, "driver version 1.0 loaded\n");

// 设备相关速率限制
dev_err_ratelimited(dev, "interrupt timeout (count=%d)\n", count);
dev_warn_ratelimited(dev, "rx queue full\n");
```

### netdev_dbg / netif_* 系列

```c
// 网络设备专用
netdev_dbg(netdev, "tx: skb=%p len=%u\n", skb, skb->len);
netdev_err(netdev, "link down\n");

// 网络子系统消息
netif_err(netdev, probe, dev, "failed to initialize\n");
netif_warn(netdev, link, dev, "link timeout\n");
netif_dbg(netdev, drv, dev, "resume complete\n");
```

### dev_dbg vs pr_debug

| 特性 | pr_debug | dev_dbg |
|------|----------|---------|
| 设备名前缀 | ❌ | ✅ 自动 |
| 上下文要求 | 任何 | 需 `struct device *` |
| dyndbg 支持 | ✅ | ✅ |
| 适用 | 通用代码 | 驱动代码 |

### HFT 关联

HFT 自定义 PCIe 设备驱动应统一使用 `dev_dbg()`，在日志中自动包含设备名，便于多设备实例区分。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** dev_dbg 如何获取设备名？输出格式是什么？

> `dev_dbg()` 从传入的 `struct device *` 中提取 `dev_name(dev)`（通常是总线地址或设备树节点名）。输出格式为 `dev_name: message`。例如 PCI 设备 `0000:01:00.0: config: 0x1234`。

**Q2:** `netif_dbg(tp, drv, dev, ...)` 中的第二个参数是什么？

> 是消息分类 (msglevel)。网络子系统支持按分类过滤消息：drv（驱动）、probe（探测）、link（链路）、ifdown（接口关闭）等。通过 `ethtool -s eth0 msglevel drv on` 动态控制。比 dyndbg 更细粒度但仅适用于网络设备。


**Q:** dev_dbg() 在未启用 Dynamic Debug 时是否有开销？

> 几乎无开销——编译为空语句（if 0 分支被编译器优化掉）。只有 CONFIG_DYNAMIC_DEBUG=y 时 dev_dbg 才实际执行。这是 dev_dbg 相比 printk(KERN_DEBUG) 的优势：零开销时完全消除。

</details>

## 交叉引用

- [05.6 ch03 dynamic debug](chapter-03-printk/notes/section-3-3.md)
