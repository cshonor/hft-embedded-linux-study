# LFS 与 LKD 第三版 · 阅读关系

> 父目录：[05-Linux-Kernel-Development](./README.md)

---

## 一、子目录角色

| 子目录 | 角色 |
|--------|------|
| `01_Course_LFS` | 建立「整机构建」上下文 |
| `00_Book_3rd_Notes` | **官方体系化主线** — Love 第三版 |

LFS = 前置预习；书本 = 核心规范与源码细节。二者互补，**不是做完 LFS 就可以不读书**。

---

## 二、LFS 扫清哪些阅读障碍

| LFS 建立的能力 | 对应书本章节 |
|----------------|-------------|
| 完整编译内核、根文件系统、glibc 配合 | Ch 2 内核入门、`menuconfig`、镜像与模块安装 |
| 用户程序 vs 内核代码边界 | Ch 5 系统调用 |
| 「操作系统如何跑起来」的整体图景 | Ch 1 简介 |

---

## 三、阅读体验对比

| 路径 | 体验 |
|------|------|
| 直接啃书 | 名词/API/流程陌生，每页查资料，易放弃 |
| **先 LFS 后书** | 书本变成系统化梳理；编译/根文件系统经验降低 Ch 2 门槛 |

---

## 四、推荐两步闭环

1. **完成 LFS** — 在 `01_Course_LFS` 写完配套笔记
2. **通读 LKD 第三版** — 在 `00_Book_3rd_Notes` 按 [OUTLINE](./00_Book_3rd_Notes/OUTLINE.md) 补充书本知识点

---

## 五、HFT 额外收益

LFS + 书本重叠的 **虚拟内存、内核网络、调度、中断**，正是低延迟调优最关键的内核机制：

| HFT 主题 | 书本 |
|----------|------|
| 绑核 / RT 调度 | LKD Ch 4 |
| 中断 / softirq 抖动 | LKD Ch 7–8 |
| NUMA / 大页 | LKD Ch 12 + [03 Gorman](../07-Linux-Virtual-Memory-Manager/) |
| 无锁 / RCU | LKD Ch 9–10 |

→ 工程师落地：[15-HFT-Low-Latency-Practice](../15-HFT-Low-Latency-Practice/)
