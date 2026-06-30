# 设备树（Device Tree）· ARM 硬件描述

**文件夹 21** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线18–23)

> **定位：** 现代 ARM Linux **不用 C 硬编码寄存器** — 用 **DTB** 描述板级硬件。  
> **前置：** [20 驱动](../20-Linux-Device-Driver/) · [18 ARM64](../18-ARM64-Architecture/)

---

## 必读书（1 本 · 核心）

| # | 书目 | 读什么 |
|---|------|--------|
| 1 | **《Device Tree for Embedded Linux》** | DTS 语法 · compatible · reg/interrupt · **overlay** |

---

## 为何 HFT 工程师也要学

| 场景 | 关联 |
|------|------|
| **x86 服务器 HFT** | 多为 ACPI — **很少写 DT** |
| **ARM 网关 / 无人机 / 车载** | **必用 DT** — 换传感器只改 DTS |
| **驱动匹配** | `of_match_table` ↔ DTS `compatible` |

---

## DTS 核心概念

```dts
/* 示意 — 非完整板级 */
my_sensor: sensor@48 {
    compatible = "vendor,imu-spi";
    reg = <0x48>;
    interrupt-parent = <&gpio0>;
    interrupts = <17 IRQ_TYPE_EDGE_RISING>;
};
```

| 属性 | 含义 |
|------|------|
| **compatible** | 驱动匹配键 |
| **reg** | 寄存器/地址范围 |
| **interrupts** | IRQ 线 |
| **status = "disabled"** | 板级裁剪外设 |

---

## 与构建链衔接

```
硬件原理图 → .dts（源码）→ dtc → .dtb
                              ↓
                    U-Boot / 内核启动时传入
                              ↓
                    驱动 probe 时 of_* 解析
```

→ [19 U-Boot 构建](../19-UBoot-Kernel-Build/) · [20 驱动 probe](../20-Linux-Device-Driver/)

---

## 验收

- [ ] 能读板级 **.dts** 找到某外设的 compatible/reg/interrupts  
- [ ] 能写 **最小 DTS 节点** 并配合 platform 驱动匹配  
- [ ] 理解 **DT overlay** 在量产/迭代中的用途  

**上一章：** [20 驱动](../20-Linux-Device-Driver/) · **下一章：** [22 实战](../22-Embedded-Linux-Practice/)
