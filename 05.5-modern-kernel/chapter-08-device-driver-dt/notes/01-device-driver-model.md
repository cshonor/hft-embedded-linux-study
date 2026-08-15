# Bootlin: 设备驱动模型

> **来源:** [Bootlin Kernel Training](https://bootlin.com/docs/kernel/)
> **主题:** 设备模型 / sysfs / 内核模块
> **对标旧书:** ULK3 Ch13 / LKD3 Ch14

---

## 讲义要点

### Linux 设备模型 (6.x)

```
bus_type
├── devices/         (所有设备挂在这里)
│   └── device → driver (匹配后绑定)
├── drivers/         (所有驱动挂在这里)
│   └── driver → device (绑定的设备列表)
└── match()          (总线匹配函数)
```

### 四大核心对象

| 对象 | 结构体 | 说明 |
|------|--------|------|
| **Device** | `struct device` | 硬件设备抽象 |
| **Driver** | `struct device_driver` | 设备驱动 |
| **Bus** | `struct bus_type` | 总线类型 (platform/pci/usb/...) |
| **Class** | `struct class` | 设备功能分类 (net/block/input/...) |

### 设备树匹配 (6.x)

```c
// platform driver 设备树匹配
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "vendor,my-device", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_dt_ids);

static struct platform_driver my_driver = {
    .probe  = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my-device",
        .of_match_table = my_dt_ids,
    },
};
module_platform_driver(my_driver);
```

### sysfs 接口

```bash
# 设备模型在 sysfs 中的体现
/sys/bus/platform/devices/     # 平台设备
/sys/bus/platform/drivers/     # 平台驱动
/sys/class/net/                # 网络设备类
/sys/class/block/              # 块设备类

# 设备属性
cat /sys/class/net/eth0/mtu    # 读取设备属性
echo 1500 > /sys/class/net/eth0/mtu  # 写入设备属性
```

### 内核模块开发骨架

```c
#include <linux/module.h>
#include <linux/platform_device.h>

static int my_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct resource *res;
    void __iomem *regs;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    regs = devm_ioremap_resource(dev, res);

    int irq = platform_get_irq(pdev, 0);
    devm_request_irq(dev, irq, my_handler, IRQF_ONESHOT,
                     "my-dev", dev);

    // devm_ 自动管理资源，remove 时自动释放
    return 0;
}

static int my_remove(struct platform_device *pdev) {
    // devm_ 资源自动释放，无需手动 cleanup
    return 0;
}
```

### devm_ 资源管理 (6.x 重要变化)

| 旧方式 (手动管理) | 新方式 (devm_ 自动管理) |
|------------------|----------------------|
| `ioremap()` + `iounmap()` | `devm_ioremap_resource()` |
| `request_irq()` + `free_irq()` | `devm_request_irq()` |
| `kmalloc()` + `kfree()` | `devm_kzalloc()` |
| `clk_get()` + `clk_put()` | `devm_clk_get()` |

`devm_` 函数在设备 detach 时自动释放资源，类似用户空间的 RAII，减少资源泄漏。

---

## 动手实验

```bash
# 1. 查看设备模型
ls /sys/bus/platform/devices/ | head -20
ls /sys/bus/platform/drivers/ | head -20

# 2. 查看设备绑定
readlink /sys/bus/platform/drivers/my-device
# 显示绑定的设备

# 3. 加载/卸载内核模块
insmod my_module.ko
rmmod my_module
modprobe my_module  # 自动处理依赖

# 4. 查看模块信息
lsmod               # 已加载模块
modinfo my_module.ko  # 模块信息
cat /proc/modules   # 模块详情

# 5. 查看内核日志
dmesg | tail -20
```

---

## 与旧书差异

| ULK3 讲的 | Bootlin 讲义 |
|-----------|-------------|
| 手动资源管理 | devm_ 自动管理 |
| 无设备树 | 设备树是嵌入式核心 |
| `request_irq()` | `devm_request_irq()` |
| `ioremap()` + `iounmap()` | `devm_ioremap_resource()` |
| 手动 platform_driver_register | `module_platform_driver()` 宏 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `devm_` 系列函数的优势是什么？

> 类似用户空间 C++ 的 RAII，在设备 detach (remove) 时自动释放资源。不需要在 remove 函数中手动调用 iounmap/free_irq/kfree 等，减少资源泄漏。特别是在错误处理路径（probe 部分失败时），不需要复杂的 goto 清理链。

**Q2:** 设备树的 `compatible` 属性如何匹配驱动？

> 内核在启动时解析设备树，为每个设备树节点创建 platform_device。驱动注册时提供 `of_match_table`，内核用设备树节点的 `compatible` 字符串匹配 `of_match_table` 中的 `.compatible`。匹配成功后调用驱动的 `probe()` 函数。

**Q3:** `module_platform_driver()` 宏做了什么？

> 它展开为 `module_init` / `module_exit`，自动调用 `platform_driver_register()` 和 `platform_driver_unregister()`。省去手写 init/exit 函数，减少样板代码。类似的宏还有 `module_pci_driver()`、`module_i2c_driver()` 等。

</details>
