# 05 · rootfs：让内核有东西可以 exec

> **本节讲什么：** 内核启动最后一步要跑 `init`，而 `init` 必须来自某个文件系统。这一节做出来这个文件系统。
> **对应动手：** [P5 · B2](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（最小 rootfs，串口进 shell，理解 init）

---

## 一个 rootfs 最少需要什么

| 组成 | 作用 |
|------|------|
| `/sbin/init`（或 `/init`） | PID 1，内核启动的终点、用户空间的起点 |
| `busybox` | 提供 `sh` + 一百多个常用命令（单二进制 + 符号链接） |
| `/dev` | `console`、`null`、`ttyAMA0`… 否则 shell 打不开终端 |
| `/lib` + `/lib/modules` | C 库与内核模块（见 [04-kernel-build](../04-kernel-build/)） |
| `/etc/inittab`（BusyBox init） | 定义开哪些 getty、跑哪些启动脚本 |

---

## 三条路怎么选

| 方式 | 适合 | 说明 |
|------|------|------|
| **initramfs** | 调试、早期 bring-up | 直接编进内核（`CONFIG_INITRAMFS_SOURCE`），无需块设备；改动要重新编内核 |
| **Buildroot** | **本模块主线** | Kconfig + make，与内核同构；几十分钟出可启动镜像 |
| **Yocto** | 量产、长期维护 | recipe/layer/bitbake 另学一套；几小时 + 几十 GB。**学习阶段不要碰** |

### Buildroot vs Yocto（为什么主线走 Buildroot）

| | Buildroot | Yocto |
|--|-----------|-------|
| 首个可启动镜像 | 几十分钟 | 数小时起 |
| 磁盘占用 | 几个 GB | 几十 GB |
| 概念栈 | Kconfig + make（**与内核同构，学一次用两次**） | recipe / layer / bitbake |
| 产物 | 一个完整镜像 | 一个可长期演进的发行版 |
| 适合 | 搞懂启动链、快速迭代 | 多机型、产品化、需要包管理 |

> Yocto 是量产的**事实标准**，但不是学习入口。等真要做产品镜像再上，别让它占掉主线时间。

---

## 关键命令

```bash
# Buildroot 快速路线
git clone https://github.com/buildroot/buildroot && cd buildroot
make raspberrypi5_defconfig          # 或 qemu_aarch64_virt_defconfig 练手
make menuconfig                       # 按需加包、设 root 密码、选 init
make -j$(nproc)
ls output/images/                     # sdcard.img / rootfs.tar / Image
```

**最小手工 rootfs（理解用）：**

```bash
mkdir -p rootfs/{bin,sbin,dev,etc,proc,sys,lib}
cp $(which busybox) rootfs/bin/        # 静态编译版更省事
ln -s busybox rootfs/bin/sh
sudo mknod rootfs/dev/console c 5 1
```

---

## HFT / 嵌入式关联

- **启动即恢复**：HFT 网关掉线按秒计费，rootfs 越小、init 越简单，重启越快。这条线后面接到 [07-storage-ota](../07-storage-ota/)（只读 rootfs + A/B 分区）。
- **最小化 = 确定性**：rootfs 里少一个守护进程，就少一个可能抢 CPU、抖 p99 的源头。

---

## 本节笔记

| 篇 | 主题 |
|----|------|
| [5.1 手工最小 rootfs](./5.1-minimal-rootfs-by-hand.md) | 内核启动到底需要什么；六个必需项；可直接抄的流程 |
| [5.2 initramfs](./5.2-initramfs.md) | 与 initrd 的本质区别；`init_eaccess("/init")` 判据；三种打包 |
| [5.3 Buildroot 第一次出镜像](./5.3-buildroot-first-image.md) | 目录结构、四个定制手段、故障对照 |

---

## 验收

- [ ] 用 Buildroot 出过一个能启动的镜像
- [ ] 手工搭过最小 rootfs，能解释每个必需目录的作用
- [ ] 串口或 SSH 进过 shell，PID 1 是什么说得出来
- [ ] 能说清 Buildroot 与 Yocto 的取舍理由

---

## 衔接

- **上一步：** [04-kernel-build](../04-kernel-build/)
- **下一步：** [06-boot-to-shell](../06-boot-to-shell/)
- **卡住查书：** MELP Ch5–6 · Primer Ch6、Ch9、Ch11
