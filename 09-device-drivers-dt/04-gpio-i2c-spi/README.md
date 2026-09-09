# 04 · GPIO / I2C / SPI：接真实外设

> **本节讲什么：** 让驱动去跟板子外面的芯片说话——点灯、读传感器、驱动 SPI 屏。
> **对应动手：** [P5 · C2](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（I2C 或 SPI 从设备驱动，`compatible` 匹配、`reg`/`interrupts` 来自 DT）

---

## 三条路的分工

| 总线 | 速率量级 | 典型用途 | 内核框架 |
|------|---------|---------|---------|
| **GPIO** | 单比特 | 点灯、按键、片选、复位线 | gpiod（`gpiod_get` / `gpiod_set_value`） |
| **I2C** | 100k–1M bps | 传感器（温湿度、IMU）、EEPROM、RTC | `i2c_driver` / `i2c_client` |
| **SPI** | 几十 Mbps | 高速 ADC、显示屏、Flash | `spi_driver` / `spi_device` |

---

## GPIO（现代 gpiod 接口）

```c
struct gpio_desc *led;

led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
gpiod_set_value(led, 1);
```

DTS 侧：

```dts
mydev {
    compatible = "wzp,my-device";
    led-gpios = <&gpio 17 GPIO_ACTIVE_HIGH>;   /* gpiod_get(..., "led") 找的就是 led-gpios */
};
```

**别再用老的 `gpio_request` + 整数编号 API**，也别用 `/sys/class/gpio`（已废弃）。

---

## I2C 从设备

```c
static int my_probe(struct i2c_client *client,
                    const struct i2c_device_id *id)
{
    /* client->addr 就是 DTS 里的 reg */
    u8 val;
    i2c_smbus_read_byte_data(client, 0x00);
    /* 多寄存器场景用 regmap（Madieu Ch9），别手撕 i2c_transfer */
    return 0;
}

static const struct of_device_id my_of_match[] = {
    { .compatible = "wzp,my-sensor" }, { }
};
```

DTS 侧挂在控制器下：

```dts
&i2c1 {
    sensor@48 {
        compatible = "wzp,my-sensor";
        reg = <0x48>;          /* I2C 地址 */
    };
};
```

---

## HFT / 嵌入式关联

- **传感器采集是"退路"也是"入口"**：工业网关 / 飞控要接 SPI/I2C 传感器，这是嵌入式最稳的就业面。
- **速率对比值得记**：I2C 慢但省引脚，SPI 快但占 4 根线。选型本质是**引脚预算 vs 带宽预算**的权衡，和 HFT 里"网卡带宽 vs PCIe 通道数"是同一类问题。
- 高频采样（IMU）会直接进 [06-dma-mmap](../06-dma-mmap/) 和 [10-motion-control](../../10-motion-control) 的地盘。

---

## 本节笔记

- [4.1 · GPIO 基础](./4.1-gpio-basics.md)
- [4.2 · 排针与 DTS 的对应](./4.2-gpio-header-vs-dts.md)

---

## 验收

- [ ] 用 gpiod 点过灯（不是 sysfs 老接口）
- [ ] 写过 I2C 或 SPI 从设备驱动，真实读到传感器数据
- [ ] 地址/中断号全部来自 DT，无硬编码
- [ ] 知道 regmap 解决什么问题（至少知道该什么时候用它）

---

## 衔接

- **上一步：** [03-platform-dt](../03-platform-dt/)
- **下一步：** [05-irq-locking](../05-irq-locking/)
- **卡住查书：** Madieu Ch7–8、Ch14–15 · LDD3 Ch9
