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

软件名：**Raspberry Pi Imager**（官方烧录器；不是别的「镜像/五菱」类山寨名）。

装好即可；**先不要点写入**。

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
