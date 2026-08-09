# P5c — I2C/SPI 传感器驱动 + 设备树

> 给一个真实传感器写内核驱动，用设备树描述硬件，让用户态能读到温度/加速度。
> **做法：项目驱动，[`09`](../../../09-device-drivers-dt/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [驱动理论 refs](../../../09-device-drivers-dt/refs/classic-driver-theory/) | LDD3：file_operations、probe/remove |
| [现代驱动实践](../../../09-device-drivers-dt/refs/modern-driver-practice/) | 平台驱动、I2C/SPI 子系统 |

---

## 项目目标

把"驱动 + 设备树"这对嵌入式 Linux 核心机制跑通：设备树描述"硬件在哪"，驱动匹配"怎么访问"，用户态通过 sysfs/字符设备读数据。

## Phase 1：设备树加节点（30 分钟）

### 做什么

在设备树里给传感器加一个节点，让 `/proc/device-tree` 能看到。

### 分步实现（以 MPU6050 I2C 为例）

1. **找 I2C 总线节点**：在 `arch/arm64/boot/dts/broadcom/bcm2712-rpi-5-b.dts` 里找 `&i2c0` 或 `&i2c1`
2. **加设备节点**：
   ```dts
   &i2c1 {
       status = "okay";

       mpu6050@68 {
           compatible = "invensense,mpu6050";
           reg = <0x68>;           // I2C 地址
           interrupt-parent = <&gpio>;
           interrupts = <17 2>;    // GPIO17, 下降沿
       };
   };
   ```
3. **编译 dtb**：`make ARCH=arm64 dtbs`
4. **拷贝到 SD 卡，启动**
5. **验证**：`ls /proc/device-tree/i2c1/mpu6050@68/` 看到 `compatible`、`reg` 等文件

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| I2C 总线没启用 | /proc 里看不到 | 加 `status = "okay"` |
| 地址错 | 驱动 probe 失败 | MPU6050 地址 0x68（AD0=0）或 0x69（AD0=1）|
| interrupt 编码错 | 中断不触发 | GPIO 编号和触发类型要查 dt-bindings |
| 修改了 dts 但没编译 | 不生效 | `make dtbs` 后拷贝新 dtb |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 设备树语法 | [MELP ch11](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-11-device-drivers-interaction/) |
| compatible 匹配 | [驱动理论 refs](../../../09-device-drivers-dt/refs/classic-driver-theory/) |

---

## Phase 2：平台驱动 + probe（1-2 小时）

### 做什么

写一个 I2C 驱动，匹配设备树里的 `compatible`，probe 时读芯片 ID。

### 代码骨架

```c
// src/mpu6050.c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#define MPU6050_WHOAMI  0x75
#define MPU6050_ID      0x68

struct mpu6050_data {
    struct i2c_client *client;
    struct regmap *regmap;
};

static const struct regmap_config mpu6050_regmap_cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x75,
};

static int mpu6050_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct mpu6050_data *data;
    unsigned int chip_id;
    int ret;

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data) return -ENOMEM;

    data->client = client;
    data->regmap = devm_regmap_init_i2c(client, &mpu6050_regmap_cfg);
    i2c_set_clientdata(client, data);

    // 读 WHO_AM_I 寄存器验证芯片
    ret = regmap_read(data->regmap, MPU6050_WHOAMI, &chip_id);
    if (ret) {
        dev_err(&client->dev, "failed to read chip ID\n");
        return ret;
    }

    if (chip_id != MPU6050_ID) {
        dev_err(&client->dev, "chip ID 0x%02x, expected 0x%02x\n",
                chip_id, MPU6050_ID);
        return -ENODEV;
    }

    dev_info(&client->dev, "MPU6050 detected, ID=0x%02x\n", chip_id);
    return 0;
}

static void mpu6050_remove(struct i2c_client *client)
{
    // devm_ 分配的资源自动释放
    dev_info(&client->dev, "MPU6050 removed\n");
}

static const struct of_device_id mpu6050_of_match[] = {
    { .compatible = "invensense,mpu6050" },
    { }
};
MODULE_DEVICE_TABLE(of, mpu6050_of_match);

static struct i2c_driver mpu6050_driver = {
    .probe   = mpu6050_probe,
    .remove  = mpu6050_remove,
    .driver  = {
        .name = "mpu6050",
        .of_match_table = mpu6050_of_match,
    },
};
module_i2c_driver(mpu6050_driver);

MODULE_LICENSE("GPL");
```

### 分步实现

1. **`of_device_id` 表**：`compatible` 必须跟设备树里写的一致
2. **`probe`**：匹配成功时调用，读 WHO_AM_I 验证芯片
3. **`regmap`**：统一寄存器访问接口（I2C/SPI 通用）
4. **`devm_` 前缀**：自动管理资源，remove 时自动释放
5. **`module_i2c_driver`**：一行宏替代 init/exit 样板代码
6. **编译加载**：`make && sudo insmod mpu6050.ko` → `dmesg` 看到 "MPU6050 detected"

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| compatible 不匹配 | probe 不调用 | dts 和 of_match_table 必须完全一致 |
| I2C 地址不对 | read 返回错误 | 查芯片 datasheet 确认 7-bit 地址 |
| 用 i2c_transfer 不用 regmap | 代码冗长 | regmap 封装了 I2C 读写的细节 |
| 忘了 MODULE_DEVICE_TABLE | 模块不能自动加载 | 需要 of 匹配表 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 平台驱动模型 | [驱动理论 refs](../../../09-device-drivers-dt/refs/classic-driver-theory/) |
| I2C 子系统 | [现代驱动实践](../../../09-device-drivers-dt/refs/modern-driver-practice/) |
| probe/remove 生命周期 | [LKD 17.2 设备模型](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-17-devices-modules/notes/section-17.2-统一设备模型.md) |

---

## Phase 3：用户态读取传感器数据（1 小时）

### 做什么

在 probe 里注册 iio 设备或字符设备，用户态能读到加速度/温度。

### 分步实现

1. **读传感器数据**：用 `regmap_bulk_read` 一次读 6 字节加速度 (X/Y/Z 各 2 字节)
2. **注册字符设备**（复用 P4 经验）或 iio 设备
3. **用户态**：`cat /dev/mpu6050` 或 `cat /sys/bus/iio/devices/iio:device0/in_accel_x_raw`
4. **测试**：晃动板子，看数值变化

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| iio 子系统 | [现代驱动实践](../../../09-device-drivers-dt/refs/modern-driver-practice/) |
| 字符设备注册 | P4 Part A（你已经做过了！）|

---

## Phase 4：中断驱动 + 连续采样（1 小时）

### 做什么

用传感器的 DRDY 引脚触发中断，中断里读数据，避免轮询。

### 分步实现

1. **设备树加 interrupt**（Phase 1 已加）
2. **驱动里 `devm_request_threaded_irq`**：
   ```c
   ret = devm_request_threaded_irq(&client->dev, client->irq,
       mpu6050_irq_handler,    // 顶半部：只确认中断
       mpu6050_irq_thread,     // 线程化底半部：读数据
       IRQF_TRIGGER_FALLING, "mpu6050", data);
   ```
3. **顶半部**：返回 `IRQ_WAKE_THREAD`，唤醒底半部
4. **底半部**：`regmap_bulk_read` 读加速度，存到 ring buffer（复用 P2.5 的无锁队列）
5. **用户态连续读**：阻塞读或 poll

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 中断号不对 | 注册失败 | 用 `client->irq`（从设备树解析）|
| 顶半部耗时太多 | 影响系统响应 | 顶半部只做最小操作，重活放线程 |
| 线程化中断不触发 | 没数据 | 检查 IRQF_TRIGGER_FALLING 是否匹配硬件 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 内核中断 | [ULK ch04 中断](../../../20-linux-kernel-deep/chapter-04-interrupts-and-exceptions/) |
| threaded_irq | [现代驱动实践](../../../09-device-drivers-dt/refs/modern-driver-practice/) |

---

## 测试验证

```bash
# 加载驱动
sudo insmod mpu6050.ko
dmesg | grep mpu6050    # 看到 "MPU6050 detected"

# 读数据
cat /sys/bus/iio/devices/iio:device0/in_accel_x_raw
# 晃动板子，数值变化 → 成功

# 查看设备树
ls /proc/device-tree/i2c1/mpu6050@68/
```

## 状态

⬜ 未开始 → 建议先确认手上有传感器模块（MPU6050 或 BMP280），然后在设备树里加节点。

← [P5 索引](../README.md) · [12 模块](../../../09-device-drivers-dt/)
