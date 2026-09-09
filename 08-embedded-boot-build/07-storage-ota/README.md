# 07 · 存储与 OTA：设备出场之后怎么活

> **本节讲什么：** 选存储介质、设计分区、让系统在无人值守环境下安全升级。
> **为什么放最后：** 这一节是"能跑"到"能交付"的分界线。前面六节让板子起来，这一节让它能在现场活下去。

---

## 介质选型

| 介质 | 特点 | 注意 |
|------|------|------|
| **microSD** | 便宜、可换 | 有写入寿命，掉电易损坏文件系统。**不适合写入密集** |
| **eMMC** | 焊死、比 SD 可靠 | 工业级首选 |
| **NOR/NAND Flash** | 走 MTD 子系统 | 需要 `UBI`/`UBIFS`，不是块设备，不能用 ext4 |
| **NVMe（Pi 5 有 PCIe）** | 快、可做根分区 | Pi 5 可经 RP1 的 PCIe 挂 NVMe，需配置启动顺序 |

**文件系统：** ext4（块设备通用） · squashfs（只读压缩，防篡改） · overlayfs（只读底 + 可写上层） · UBIFS（裸 Flash）。

---

## 分区策略：为什么产品都用只读 rootfs

```
[ boot (FAT) ] [ rootfs A (ro) ] [ rootfs B (ro) ] [ data (rw) ]
                    ↑ 当前           ↑ 升级目标
```

| 设计 | 目的 |
|------|------|
| **只读 rootfs** | 掉电不损坏；防误改；与"配置与代码分离"同构 |
| **A/B 双分区** | 升级写到备用分区，失败自动回退 |
| **独立 data 分区** | 日志与配置独立，升级不丢 |
| **overlayfs** | 需要"看起来可写"时，用内存层承接写操作 |

---

## 关键命令

```bash
# 看分区与挂载
lsblk -f
findmnt -o TARGET,SOURCE,FSTYPE,OPTIONS

# overlay 结构
mount -t overlay overlay -o lowerdir=/ro,upperdir=/rw/upper,workdir=/rw/work /merged

# 只读切换（救急）
mount -o remount,ro /
```

---

## HFT / 嵌入式关联

- **只读 rootfs 是"不可变基础设施"在嵌入式的形态**：HFT 生产机上，交易二进制通常也放在只读挂载点，防止运行期被改。
- **A/B 升级 + 快速回退** 对应交易系统的灰度发布：新版本出问题要能在一次心跳内回退，而不是现场重刷。
- 掉电保护这件事在嵌入式是物理约束，在 HFT 机房是 UPS + 持久化日志——**问题同构，解法不同**，值得对照着看。

---

## 验收

- [ ] 能说出 microSD / eMMC / NAND 各自的适用边界
- [ ] 知道为什么 NAND 不能用 ext4
- [ ] 画得出 A/B + data 的分区图，并说清升级流程
- [ ] 搭过一次 overlayfs，理解 lower/upper/work 三层

---

## 衔接

- **上一步：** [06-boot-to-shell](../06-boot-to-shell/)
- **下一步：** [09-device-drivers-dt](../../09-device-drivers-dt) — 开始写驱动
- **卡住查书：** MELP Ch9–10 · Primer Ch10（MTD）
- **延伸：** 电源管理 MELP Ch15 · 实时性 MELP Ch21 / Primer Ch17
