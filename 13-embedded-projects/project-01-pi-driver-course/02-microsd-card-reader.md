# Project #1 · 板子准备：microSD + 读卡器

> 实物登记 · 刷机前置 · 对齐 [RASPBERRY-PI5-LABS §1.5](../RASPBERRY-PI5-LABS.md)

---

## 这两件是什么

| # | 实物 | 就是 |
|---|------|------|
| 1 | 黑色小东西 | **USB 读卡器**（川宇 / Kawasau，约 `KUW204`） |
| 2 | 小卡片 | **32GB SanDisk Ultra microSD**（microSDHC · UHS‑I · Class 10 · U1 · **A1**） |

树莓派用这张卡当「硬盘」：系统、rootfs、你的实验都写在上面。  
32GB 做驱动课 / Labs Phase A–C 足够；以后内核树、大量日志再换更大卡。

---

## 第一步：怎么插（先卡再读卡器）

1. 把 microSD **金手指（金属触点）朝里**，塞进读卡器侧面的小插槽，**按到底卡紧**。  
   - 不要硬怼；方向反了会插不进去。  
2. 再把读卡器的 **USB 金属头**插到电脑 USB 口。  
3. 等系统识别到这张存储卡（资源管理器里多出一个可移动磁盘即可）。

```
microSD ──插入──▶ 读卡器侧面插槽 ──USB──▶ 电脑
```

---

## 第二步：下载官方烧录工具

浏览器打开：

https://www.raspberrypi.com/software/

页面标题是「树莓派软件」——**到对了**。软件名：**Raspberry Pi Imager**（官方烧录器）。

### 在这一页怎么点

1. Cookie 条随便：**拒绝可选的 Cookie** 或接受都行，不影响下载。  
2. **往下滚**（粉红大标题下面还有内容），找到 **Raspberry Pi Imager** 区块。  
3. 你是 Windows → 点 **Download for Windows**（下载 `.exe`）。  
   - 直链备用：https://downloads.raspberrypi.com/imager/imager_latest.exe  
4. 跑安装包：  
   - **Select Destination Location** → 默认 `C:\Program Files\Raspberry Pi Ltd\Imager` 即可 → **Next**（一般不用改路径）。  
   - 后面若问开始菜单/桌面快捷方式，按默认勾选继续 → 装完即可。  
5. 从开始菜单打开 **Raspberry Pi Imager**。  

**先不要点写入。**

### Imager 提示 Offline / Unable to download OS list

窗口标题带 **Offline**，或黄条 **Unable to download OS list**，说明软件拉不到官方系统列表（网络/代理/防火墙）。

| 别点 | 原因 |
|------|------|
| **格式化** | 只会把卡弄成空 FAT32，**装不上**树莓派系统 |

**优先：恢复联网列表（省事）**

1. 确认电脑能打开 https://www.raspberrypi.com （浏览器能上官网）  
2. 关 VPN / 系统代理试一下；公司网若拦 downloads.raspberrypi.com 换手机热点  
3. 点左下角 **应用选项**，看有无代理相关设置；改完**关掉 Imager 再开**  
4. 能联网后应出现完整 OS 列表（不再只有「格式化 / 自定义镜像」）→ 再按正常流程选 **Pi 5 + Lite 64-bit**

**备选：本地下镜像（继续 Offline）**

1. 浏览器打开：https://www.raspberrypi.com/software/operating-systems/  
2. 找到 **Raspberry Pi OS Lite** 的 **64-bit**，下载 `.img.xz`（约几百 MB）  
3. Imager 里点 **使用自定义镜像** → 选刚下的文件（一般**不用先解压**，Imager 认 `.xz`）  
4. 再选储存设备（你的 SD 读卡器）→ 自定义设置里开 SSH → 写入  

直链目录（版本会变，以官网页为准）：  
https://downloads.raspberrypi.com/raspios_lite_arm64/images/

**先不要点写入**；也先别在官网单独下 OS——**仅当 Offline 走不通联网列表时**才用「自定义镜像」。

---

## 本步完成标准（到这里停）

- [ ] 卡已正确插入读卡器（金手指朝里、卡紧）  
- [ ] 读卡器已插上电脑，系统能看到盘符  
- [ ] Raspberry Pi Imager 已下载并安装  

三项都勾上再继续「选 Pi5 / 选 OS / 写卡」。

---

## 重要：不要手动格式化

**现在不要去点「格式化」**，也不要自己用磁盘工具清分区。  
Imager 写入时会自动处理分区与文件系统；手动格式化容易多一步坑、浪费时间。

---

## 和驱动课的关系（心里有数即可）

```
读卡器写镜像（一次性） → Pi 能 SSH / 串口登录
        ↓
再谈交叉编译、.ko、GPIO（课内主线）
```

卡和读卡器不是驱动学习对象，只是让板子活起来。三层图见 [01](./01-userspace-kernel-hardware.md)。

官网刷机细节（查选用）：[Install using Imager](https://www.raspberrypi.com/documentation/computers/getting-started.html)
