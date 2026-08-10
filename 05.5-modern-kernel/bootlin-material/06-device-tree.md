# Bootlin: 设备树 (Device Tree)

> **来源:** [Bootlin Device Tree Training](https://bootlin.com/docs/device-tree/)
> **主题:** 设备树语法与实践
> **对标旧书:** ULK3 无概念 / LKD3 简略提及

---

## 讲义要点

### 设备树基础语法

```dts
/dts-v1/;

/ {
    model = "Raspberry Pi 5 Model B";
    compatible = "raspberrypi,5-model-b", "brcm,bcm2712";

    #address-cells = <2>;
    #size-cells = <2>;

    cpus {
        cpu@0 {
            device_type = "cpu";
            compatible = "arm,cortex-a76";
            reg = <0x0 0x0>;
            enable-method = "psci";
        };
    };

    uart@7d500000 {
        compatible = "arm,pl011", "arm,primecell";
        reg = <0x0 0x7d500000 0x0 0x1000>;
        interrupts = <GIC_SPI 121 IRQ_TYPE_LEVEL_HIGH>;
        clocks = <&clk_uart>;
        status = "okay";
    };
};
```

### 核心概念

| 概念 | 语法 | 说明 |
|------|------|------|
| **Node** | `name@address { ... };` | 设备节点 |
| **Property** | `key = value;` | 设备属性 |
| **phandle** | `&label` | 节点引用 |
| **reg** | `reg = <addr size>;` | 寄存器地址和大小 |
| **interrupts** | `interrupts = <type num trigger>;` | 中断描述 |
| **compatible** | `compatible = "vendor,model";` | 驱动匹配字符串 |
| **status** | `status = "okay"/"disabled";` | 启用/禁用设备 |

### address-cells / size-cells

```dts
// #address-cells = <2>: reg 的地址部分用 2 个 u32 (64-bit)
// #size-cells = <1>: reg 的大小部分用 1 个 u32 (32-bit)
// reg = <addr_hi addr_lo size>

// 例如: 64-bit 地址 0x000000007d500000, 大小 0x1000
uart@7d500000 {
    reg = <0x0 0x7d500000 0x1000>;  // 3 个 cell: 2 addr + 1 size
};
```

### 中断描述 (GIC 格式)

```dts
// GIC 中断格式: <type number trigger>
// type: GIC_SPI (0, 共享外设中断) / GIC_PPI (1, 私有外设中断)
// number: 中断号
// trigger: IRQ_TYPE_EDGE_RISING (1) / LEVEL_HIGH (4) / etc.

// SPI 中断 121, 电平触发高有效
interrupts = <GIC_SPI 121 IRQ_TYPE_LEVEL_HIGH>;

// PPI 中断 12 (通用定时器), 边沿触发
interrupts = <GIC_PPI 12 IRQ_TYPE_EDGE_RISING>;
```

### 设备树覆盖层 (Overlay)

```dts
// 树莓派设备树覆盖层: 启用额外硬件
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2712";

    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            status = "okay";
            #address-cells = <1>;
            #size-cells = <0>;

            sensor@48 {
                compatible = "vendor,temp-sensor";
                reg = <0x48>;
            };
        };
    };
};
```

```bash
# 编译和加载覆盖层
dtc -@ -O dtb -o my-overlay.dtbo my-overlay.dts
cp my-overlay.dtbo /boot/firmware/overlays/
# 在 config.txt 中添加: dtoverlay=my-overlay
```

---

## 动手实验

```bash
# 1. 查看当前运行的设备树
ls /sys/firmware/devicetree/base/
ls /proc/device-tree/        # 同上（符号链接）

# 2. 查看特定节点
cat /proc/device-tree/model             # "Raspberry Pi 5 Model B"
cat /proc/device-tree/compatible        # "raspberrypi,5-model-b\0brcm,bcm2712"
ls /proc/device-tree/uart@7d500000/

# 3. 反编译当前设备树
dtc -I fs /sys/firmware/devicetree/base -O dts -o current.dts
less current.dts

# 4. 查看设备树覆盖层
ls /boot/firmware/overlays/
cat /boot/firmware/config.txt | grep dtoverlay

# 5. 编译设备树源文件
dtc -O dtb -o my-board.dtb my-board.dts
```

---

## 与旧书差异

| ULK3 / LKD3 | Bootlin 讲义 |
|-------------|-------------|
| 无设备树概念 | 设备树是嵌入式核心 |
| 硬编码设备地址 | 通过 DT 描述硬件 |
| 无 overlay | DT overlay 动态启用硬件 |
| 板级代码 (arch/arm/mach-*) | 已废弃，用 DT 替代 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `#address-cells = <2>` 和 `#size-cells = <1>` 意味着 `reg` 属性有几个 cell？

> 3 个 cell：2 个用于地址（64-bit），1 个用于大小（32-bit）。例如 `reg = <0x0 0x7d500000 0x1000>` 表示地址 0x000000007d500000，大小 0x1000。

**Q2:** 设备树中 `compatible` 属性为什么可以有多个值？

> 多个值是回退匹配链：内核先尝试匹配第一个字符串，如果没找到驱动再尝试下一个。例如 `compatible = "raspberrypi,5-model-b", "brcm,bcm2712"` 先匹配树莓派 5 专用驱动，如果没有则匹配 BCM2712 通用驱动。

**Q3:** 设备树覆盖层 (overlay) 的作用是什么？

> 覆盖层允许在不修改基础设备树的情况下动态添加/修改节点。树莓派用它来按需启用扩展硬件（如 HAT 扩展板、I2C 传感器）。加载方式：编译为 `.dtbo` 文件，在 config.txt 中用 `dtoverlay=` 指定。

</details>
