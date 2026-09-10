# 03 · platform + 设备树：本模块的核心

> **本节讲什么：** 现代 ARM Linux 驱动的**主干**——硬件信息从设备树来，驱动通过 `compatible` 匹配，资源全部从 DT 解析，**基址不硬编码**。
> **对应动手：** [P5 · C3](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（platform + probe，资源全从 DT 解析，无硬编码基址，对照 RP1 / 板级 DTS）

---

## 为什么是核心

SoC 上的外设**没有总线可以枚举**（不像 PCIe 能扫出来）。所以内核必须靠一张"硬件清单"来知道有什么设备——这张清单就是**设备树**。

```
DTS（人写） --dtc--> DTB（二进制） --> bootloader 传给内核
                                            ↓
                                  展开为 device_node 树
                                            ↓
                       platform_bus 按 compatible 匹配 driver
                                            ↓
                                      driver->probe()
```

---

## DTS 节点长什么样

```dts
mydev@fe200000 {
    compatible = "wzp,my-device";     /* 匹配的钥匙 */
    reg = <0x0 0xfe200000 0x0 0x1000>;   /* 地址 + 长度 */
    interrupts = <GIC_SPI 32 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clk_peri>;              /* phandle 引用别的节点 */
    status = "okay";
};
```

驱动侧：

```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "wzp,my-device" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static int my_probe(struct platform_device *pdev)
{
    struct resource *res;
    void __iomem *base;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    base = devm_ioremap_resource(&pdev->dev, res);   /* devm_ 自动回收 */
    /* 寄存器读写：readl(base + REG_X) / writel(v, base + REG_X) */
    return 0;
}
```

**关键：** 用 `devm_*` 系列（`devm_ioremap_resource`、`devm_kzalloc`、`devm_request_irq`），出错路径不用手写 unwind，remove 时自动释放。

---

## platform_device 从哪来

| 来源 | 说明 |
|------|------|
| **设备树节点** | 现代主流。内核自动把 DT 节点转成 platform_device |
| **ACPI** | x86 世界 |
| **C 代码硬编码** | 老式（板级文件），**现在别这么写** |

---

## 本节笔记

- [3.1 · 加新硬件：改 DTS 还是写驱动](./3.1-new-hw-dts-vs-driver.md)
- [3.2 · DTS 与驱动的关系](./3.2-dts-driver-relationship.md)
- [3.3 · DTB 的一生：DTS → platform_device](./3.3-dtb-lifecycle.md) ★ 先查"跑的是哪棵树"
- [3.4 · compatible 匹配与 probe 全流程](./3.4-compatible-match-and-probe.md) ★ 为什么"没报错"最难查
- [3.5 · devm 与出错路径](./3.5-devm-and-error-paths.md)
- [UEFI ≈ U-Boot，不是 DTS · ACPI 与 DTB 才是对位关系](../../08-embedded-boot-build/01-orientation/1.6-device-tree-vs-uefi.md)

---

## 官方 DT 文档（比书权威）

| 文档 | 用途 |
|------|------|
| [Usage Model](https://docs.kernel.org/devicetree/usage-model.html) | `compatible` · 匹配机制 · FDT/DTB 启动链 |
| [Devicetree Spec](https://devicetree-specification.readthedocs.io/en/latest/usage-model.html) | 语法 · `reg`/`interrupts` · phandle |
| [Bindings 索引](https://docs.kernel.org/devicetree/bindings/index.html) | 查外设 `compatible` 该写什么 |
| [Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html) | DT overlay（`dtoverlay`，Pi 上常用） |

---

## HFT / 嵌入式关联

- **`compatible` 是契约**：厂商 DTS 与驱动的对接点。写错一个字符，probe 不调用，没有任何报错——这是嵌入式调试最阴的一类问题。
- Pi 5 的外设走 **RP1**（经 PCIe 挂的南桥），引脚和寄存器叙事与 Pi 4 不同。**第三方教程多基于 Pi 4，别照抄**。

---

## 验收

- [ ] 写过 platform 驱动，资源全部从 DT 解析，**无硬编码基址**
- [ ] 在 Pi 5 上改过 DTS 并匹配成功（`dmesg` 看到 probe）
- [ ] 能解释 DTB 从哪来、内核用来干什么
- [ ] 用过 `devm_*`，知道它解决了什么问题
- [ ] 会用 `dtc` 反编译 `/proc/device-tree`

---

## 衔接

- **上一步：** [02-char-device](../02-char-device/)
- **下一步：** [04-gpio-i2c-spi](../04-gpio-i2c-spi/)
- **卡住查书：** Madieu Ch5–6 · LDD3 Ch14
