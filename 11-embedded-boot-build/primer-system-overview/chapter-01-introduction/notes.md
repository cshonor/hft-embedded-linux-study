# Embedded Linux Primer 第 01 章 — Introduction

> 对应目录：`chapter-01-introduction/`  
> 书：*Embedded Linux Primer*, 2nd ed — Christopher Hallinan  
> 大纲：[../OUTLINE.md](../OUTLINE.md) · 标签以 OUTLINE 为准

**优先级**：1.3 GPL **精读**；1.1 选读；1.2 / 1.4 速览  
**后置**：同模块 [MELP](../../build-toolchain-yocto/)（实操落地）· 板级动手 [13 Pi Labs](../../../13-embedded-projects/RASPBERRY-PI5-LABS.md)

---

## 章节核心定位

全书开篇：解释**嵌入式为何普遍选 Linux**，分清开源里的「自由」与「免费」，扫一眼行业标准化组织，建立宏观认知。  
**不写代码**；为后文硬件、内核、Boot、rootfs 铺垫。

---

## 1.1 Why Linux?（选读）

传统专有 RTOS 在大量场景被 Linux 替代（消费电子、基站、车载、交换机、手机等）。书中归纳的优势：

| # | 优势 | 一句话 |
|---|------|--------|
| 1 | **硬件兼容面宽** | 主流处理器与外设支持远超多数商用 RTOS |
| 2 | **软件生态** | 网络协议栈、应用组件现成，少从零造轮子 |
| 3 | **可伸缩** | 可裁到小功耗设备，也可撑电信级交换机 |
| 4 | **无按设备授权费** | 省掉专有 OS 的版权/专利摊销 |
| 5 | **社区迭代** | 新平台、新外设驱动跟得快 |
| 6 | **原厂 BSP** | 主流 SoC 厂几乎都给 Linux 配套包 |

对本仓库：树莓派 / ARM 板子走 Linux，正是吃这套优势；驱动与启动仍要自己学（[12](../../../12-device-drivers-dt/) · [11](../../)）。

**扩展精读（优势 / 短板 / PREEMPT_RT / 混合架构）：**  
[1.1-linux-vs-rtos.md](./1.1-linux-vs-rtos.md) — Linux 替代 RTOS 的动因、边界与选型口诀。

---

## 1.2 Embedded Linux Today（速览）

书中时代背景（LinuxDevices 等调研，成书年代）：

- 近半数新项目选 Linux；传统商用 RTOS 占比很小  
- 大量企业**自研定制发行版**，没有单一「商用嵌入式 Linux 产品」统计口径  
- 手机、电视、路由、车机等出货量持续放大  

**读法：** 知「已成主流」即可；具体数字不必背，市场格局已变但结论仍成立。

---

## 1.3 Open Source and the GPL（精读）

### 1.3.1 Free as in Freedom vs Free as in Beer

| 说法 | 含义 | 嵌入式含义 |
|------|------|------------|
| **Freedom（自由）** | 跑、读、改、再分发；GPL 的核心 | Linux 的本质是权利，不只是零元购 |
| **Beer（免费）** | 源码可零成本下载 | **≠ 项目零成本**：工具链、移植、调试、维护、支持都要钱 |

**GPL 传染（分发触发）：** 向外部分发集成/修改过的 GPL 代码时，须按 GPL 提供对应源码（及相同许可证条件）。只内部使用、不分发，义务通常不触发——产品出货才是敏感点。

### 1.3.2 GPL 其他要点

1. **协议自延续**：衍生作品继续受 GPL；全体版权人一致换协议几乎不可能  
2. **可以卖钱**：可售卖搭载 GPL 软件的硬件/发行版，但交付时须履行源码义务  
3. **无担保**：软件「按现状」提供，无官方稳定性/可靠性保修条款  

**自检：** 能口述「自由 ≠ 免费」；能说出「出货分发时 GPL 要你交什么」。

---

## 1.4 Standards and Relevant Bodies（速览）

| 组织 / 规范 | 书中角色 | 今日怎么对待 |
|-------------|----------|--------------|
| **LSB**（Linux Standard Base） | 统一发行版二进制/库/命令/目录/init，多架构 | **工程上可忘掉**；为何书还讲、与 POSIX 分层见 [1.4-lsb-vs-posix.md](./1.4-lsb-vs-posix.md) · [TLPI Ch1](../../../04-linux-userspace-api/chapter-01-introduction/) |
| **Linux Foundation** | 非营利联盟；资助内核与生态工作组 | 知存在即可；规范多在专项组 |
| **CGL**（Carrier-Grade Linux） | 电信级：HA、集群、运维、性能、合规、硬件、安全 | 做基站/核心网时再翻；Pi 驱动课不需要 |
| **Moblin → MeeGo** | 便携设备工作组（工具、IO、内存、多媒体、功耗…） | **历史名词**；当代看 Android / 各 SoC 厂商栈 |
| **SA Forum** | 电信/工业 RAS（可靠性·可用性·可服务性）接口 | 工业高可用项目按需 |

七大 CGL 方向（速览备忘）：高可用 · 集群 · 可运维 · 性能（含 SMP/调度延迟）· 标准化 · HA 硬件 · 安全。

---

## 1.5 Summary

1. 嵌入式采用 Linux 的速度与广度持续上升（书中结论至今仍成立）  
2. 兼容性、生态、裁剪、无授权费、社区、原厂 BSP 共同推动从专有 OS 迁移  
3. **GPL：分清自由与免费**；分发义务是产品合规关键  
4. 行业组织做过标准化努力；嵌入式日常更依赖厂商 BSP + 自建 rootfs，而非 LSB  

### 1.5.1 书中推荐延伸

| 读物 | 用途 |
|------|------|
| Eric S. Raymond, *The Cathedral and the Bazaar* | 开源协作文化 |
| LSB / Linux Foundation 官网 | 查规范原文（非必读） |

---

## 与本仓库的咬合

| 本章收获 | 下一步 |
|----------|--------|
| 为何板子跑 Linux、GPL 底线 | **Ch2 Big Picture**（上电→Boot→内核→init）精读 |
| Linux vs RTOS 边界 | [1.1-linux-vs-rtos](./1.1-linux-vs-rtos.md) → 日后 [Ch17](../chapter-17-linux-and-real-time/notes.md) |
| 动手 | [Project #1 / Pi Labs](../../../13-embedded-projects/RASPBERRY-PI5-LABS.md) 刷卡上板 |
| 驱动深入 | [12 设备驱动](../../../12-device-drivers-dt/)；本书 Ch8 仅入门 |

---

## 参考

- Hallinan, *Embedded Linux Primer*, 2nd ed, Chapter 1  
- 大纲标签：[../OUTLINE.md](../OUTLINE.md) §第 1 章
