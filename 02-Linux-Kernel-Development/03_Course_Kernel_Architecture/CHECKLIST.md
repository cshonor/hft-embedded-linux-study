# 架构理论课 · 学习清单

> [03_Course_Kernel_Architecture](./README.md) · *Complete Kernel Guide*

---

## 开课前

- [ ] 已看或并行 [01 LFS](../01_Course_LFS/) 至少 p0、p11–p13（启动 + 内核）
- [ ] 已看或并行 [02 内核编程](../02_Course_Kernel_7Lectures/) 至少 e1–e2（编译 + 模块）
- [ ] 本课**不要求**每讲都敲代码；重点是画图、记概念、串体系

---

## Part I 基础（必做）

- [ ] **a01 Unix DNA** — 可移植/多任务/多用户；简单/模块化/可复用
- [ ] **a02 宏内核 vs 微内核** — Linux 为何是 monolithic；与 Windows/macOS 对比直觉
- [ ] **a03 内核架构总览** — 用户态/内核态、系统调用边界、子系统地图
- [ ] a04 固件引导 — 与 [LFS p0](../01_Course_LFS/episode-p00-概述.md) 启动链对照

**避坑：** 跳过 a01–a03 直接看 SMP/同步 → 容易「记名词不理解设计取舍」。

---

## Part II 硬件与执行（HFT 相关）

- [ ] **a05 SMP/NUMA** —  socket、内存本地性 → 绑核/绑内存前置
- [ ] **a06 抢占** — 内核可抢占 vs 不可抢占、latency 含义
- [ ] **a07 同步** — spinlock/mutex/RCU 适用场景

---

## Part III 子系统（精读笔记）

- [ ] **a08 虚拟内存** — 页表、TLB、HugePage、缺页 → 对接 Gorman
- [ ] **a09 调度** — CFS、RT、`SCHED_FIFO` → 对接 LKD Ch 4
- [ ] **a10 网络栈** — 分层、softirq、NAPI → 对接 Rosen Ch 11/14

---

## 学完后自检（再读 LKD）

- [ ] 能解释：为什么 Linux 选宏内核而不是微内核
- [ ] 能画：用户态 `read()` 到内核 VFS/网络/块设备的大致路径
- [ ] 能说明：NUMA、抢占、RCU 与 HFT 延迟的关系
- [ ] 已读 [CROSS-COURSE.md](./CROSS-COURSE.md) 打通三门课

→ 进入 [00_Book_3rd_Notes](../00_Book_3rd_Notes/OUTLINE.md) 通读

---

## 与 HFT 笔记同步

| 架构课讲 | 建议补充到 |
|----------|-----------|
| a05/a08 | [10-HFT ch05/ch07](../../10-HFT-Low-Latency-Practice/) |
| a09/a06 | 绑核、RT 策略 |
| a10 | [10-HFT ch06 网络](../../10-HFT-Low-Latency-Practice/chapter-06-低延迟网络与协议优化.md) |
