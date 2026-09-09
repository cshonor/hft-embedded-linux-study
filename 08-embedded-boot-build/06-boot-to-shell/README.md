# 06 · 启动到 shell：从 PID 1 到可用系统

> **本节讲什么：** 内核挂载 rootfs 之后发生什么——PID 1 怎么选、`init` 怎么把系统拉起来、控制台从哪来。
> **对应动手：** [P5 · B2/B3](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（串口进 shell，理解 init；启动参数与 DTB 传入）

---

## 启动时序

```
start_kernel()  →  rest_init()
                      ↓
               kernel_init()   ← 找 init
                      ↓
        /sbin/init → /etc/init → /bin/init → /bin/sh   （内核的查找顺序）
                      ↓
        init 挂 /proc /sys，按 inittab 或 unit 起服务
                      ↓
               getty 开控制台 → 登录 shell
```

内核通过 `init=` 启动参数可以指定别的 PID 1（如 `init=/bin/sh` 是最常用的救砖手段）。

---

## init 三选一

| init | 特点 | 何时用 |
|------|------|--------|
| **BusyBox init** | 读 `/etc/inittab`，几十 KB | 最小系统、恢复环境、理解原理 |
| **systemd** | 并行启动、依赖图、journal | 发行版/产品（Pi OS 默认） |
| **runit / s6** | 极简 supervise，适合容器与嵌入式 | 想要确定性又不想上 systemd |

**BusyBox `inittab` 的样子：**

```
::sysinit:/etc/init.d/rcS
::respawn:/sbin/getty -L 115200 ttyAMA0 vt100
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
```

---

## 关键命令

```bash
# 看启动耗时分布（systemd）
systemd-analyze
systemd-analyze blame | head -20
systemd-analyze critical-chain

# 看内核启动消息（含 early boot）
dmesg -T | head -60

# 谁是 PID 1
ps -p 1 -o pid,comm,args
cat /proc/cmdline
```

---

## HFT / 嵌入式关联

- `systemd-analyze critical-chain` 是**启动耗时的火焰图入口**，与后面 [06.6-systems-performance](../../06.6-systems-performance/) 的方法论同源：先量化，再优化。
- 生产环境常把关键进程做成 **PID 1 直管的最小 init**（不要 systemd 的不确定性），或干脆用 `init=/opt/app` 直接跑交易进程——这是"去掉一切不必要"的极致形态。

---

## 验收

- [ ] 能默出内核查找 init 的顺序
- [ ] 用 `init=/bin/sh` 救过一次系统（或至少在 QEMU 里练过）
- [ ] 写过 `inittab`，知道 `respawn` 与 `sysinit` 的区别
- [ ] 能说出系统启动时间里主要耗在哪一段

---

## 衔接

- **上一步：** [05-rootfs](../05-rootfs/)
- **下一步：** [07-storage-ota](../07-storage-ota/)
- **卡住查书：** MELP Ch13–14 · Primer Ch6
- **驱动侧：** 设备节点由 udev/mdev 创建 → [09 · 01-hello-module](../../09-device-drivers-dt/01-hello-module/)
