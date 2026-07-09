# 设备树（Device Tree）· ARM 硬件描述

**文件夹 22** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** 现代 ARM Linux **不用 C 硬编码寄存器** — 用 **DTB** 描述板级硬件。  
> **前置：** [21 驱动](../21-Linux-Device-Driver/) · [19 ARM64](../19-ARM64-Architecture/) · [20 U-Boot/构建](../20-UBoot-Kernel-Build/)  
> **资料：** **不另找书** — 用 **Linux 内核官方 Device Tree 文档**（与 19–21 顺接）。

---

## 官方文档（唯一必读 · 覆盖本章要点）

| # | 文档 | 读什么 |
|---|------|--------|
| **1** | [**Linux and the Devicetree**（Usage Model）](https://docs.kernel.org/devicetree/usage-model.html) | DT 数据模型 · **`compatible`** 匹配 · **platform 识别** · 驱动 **probe/population** · FDT/DTB 与启动链 |
| **2** | [Devicetree Specification — Usage](https://devicetree-specification.readthedocs.io/en/latest/usage-model.html) | **DTS 语法** · node/property · **`reg` / `interrupts`** · phandle · 标准 bindings 写法 |
| **3** | [Device Tree Bindings 索引](https://docs.kernel.org/devicetree/bindings/index.html) | 查具体外设 **`compatible`** 字符串与属性约定 |
| **选读** | [Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html) | **DT overlay** — 量产/迭代改板 |

> 内核树内同源路径：`Documentation/devicetree/usage-model.rst`（clone 内核后可离线读）。

---

## 与 19 → 20 → 21 怎么顺

```
19  ARM 汇编          → 懂 MMIO / 异常 / 与 C 互调
20  U-Boot + 内核构建  → SPL/U-Boot 加载 Image + DTB 进内核
21  LDD 驱动           → platform 驱动 of_match_table ↔ compatible
22  官方 DT 文档        → 写 .dts：reg / interrupts / status
        ↓
23  板级实战            → 改 DTS + 驱动 probe 联调
```

| 衔接点 | 说明 |
|--------|------|
| **U-Boot → 内核** | Bootloader 把 **`.dtb`** 传给内核（Usage Model §1 History） |
| **驱动 probe** | 内核用 **`compatible`** 选 `machine_desc` / 匹配 **platform driver**（§2 Platform Identification） |
| **不再硬编码** | 寄存器基址写在 **`reg`**，驱动用 **`of_*` API** 解析（→ [21](../21-Linux-Device-Driver/)） |

---

## 为何 HFT 工程师也要学

| 场景 | 关联 |
|------|------|
| **x86 服务器 HFT** | 多为 **ACPI** — 很少写 DT |
| **ARM 网关 / 无人机 / 车载** | **必用 DT** — 换传感器只改 DTS |
| **驱动匹配** | `of_match_table` ↔ DTS **`compatible`** |

---

## DTS 核心概念（对照官方文档）

```dts
/* 示意 — 非完整板级 */
my_sensor: sensor@48 {
    compatible = "vendor,imu-spi";
    reg = <0x48>;
    interrupt-parent = <&gpio0>;
    interrupts = <17 IRQ_TYPE_EDGE_RISING>;
};
```

| 属性 | 含义 | 官方文档 |
|------|------|----------|
| **`compatible`** | 驱动 / 平台匹配键 | [usage-model §2.2](https://docs.kernel.org/devicetree/usage-model.html) |
| **`reg`** | 寄存器 / 地址范围 | [Devicetree Spec · reg](https://devicetree-specification.readthedocs.io/en/latest/devicenodes.html) |
| **`interrupts`** | IRQ 线 | bindings 索引 |
| **`status = "disabled"`** | 板级裁剪外设 | 常见板级 dts 惯例 |

---

## 与构建链衔接

```
硬件原理图 → .dts（源码）→ dtc → .dtb
                              ↓
                    U-Boot / 内核启动时传入
                              ↓
                    驱动 probe 时 of_* 解析
```

→ [20 U-Boot 构建](../20-UBoot-Kernel-Build/) · [21 驱动 probe](../21-Linux-Device-Driver/)

---

## 验收

- [ ] 读过 [**usage-model**](https://docs.kernel.org/devicetree/usage-model.html) — 能解释 **DTB 从哪来、内核用来干什么**  
- [ ] 能读板级 **`.dts`** 找到某外设的 **compatible / reg / interrupts**  
- [ ] 能写 **最小 DTS 节点** 并配合 [21](../21-Linux-Device-Driver/) platform 驱动匹配  
- [ ] 知道 **DT overlay** 用途（选读 overlay-notes）

**上一章：** [21 驱动](../21-Linux-Device-Driver/) · **下一章：** [23 实战](../23-Embedded-Linux-Practice/)
