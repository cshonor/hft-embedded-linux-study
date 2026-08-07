## 9.12 小结（原书）

> **Ch9 §9.12** · [章导读](../README.md) · 上节 [§9.11 ←](./section-9.11-C程序常见内存错误.md) · 下节 —

---

← [本章导读](../README.md)

---

### Ch9 全章要点

| 主题 | 核心概念 | HFT 关联 |
|------|----------|----------|
| §9.1-9.2 | VA/PA、地址空间 | 每进程独立 VA，隔离 |
| §9.3 | VM = DRAM 缓存磁盘 | 缺页代价 μs 级，避免 |
| §9.4-9.5 | VM 简化管理 + 保护 | 共享页、PTE 权限 |
| §9.6 | 地址翻译、TLB、多级页表 | 大页减 TLB miss |
| §9.7 | i7/Linux 4 级页表 | 上下文切换刷 TLB |
| §9.8 | mmap、COW | 行情文件 mmap、大页 |
| §9.9 | malloc/free、分配器 | 对象池替代 malloc |
| §9.10-9.11 | GC、内存错误 | ASan/Valgrind、RAII |

**一句话：** 虚拟内存 = 缓存（DRAM 缓存磁盘）+ 管理（隔离/共享/简化链接）+ 保护（PTE 权限位），地址翻译由 MMU+TLB+页表完成，malloc 在用户态管理堆。

### 常见陷阱
1. **VM 不只是「内存」，是缓存+管理+保护三合一** — 不要只从「内存」角度理解
2. **malloc 实现是用户态，但最终靠 syscall 向内核要页** — malloc 不是系统调用，sbrk/mmap 才是
3. **地址翻译对程序员透明但不等于无代价** — TLB miss 时页表 walk 需要数十周期；HFT 用大页+绑核减少代价

### 自测题

<details>
<summary>Q1: 虚拟内存的三个角色分别是什么？</summary>

1) 缓存工具：DRAM 缓存磁盘（swap），按需分页；2) 管理工具：隔离进程、简化链接/加载/共享；3) 保护工具：PTE 权限位控制访问。

</details>

<details>
<summary>Q2: 地址翻译的完整路径是什么？TLB 在哪里？</summary>

VA → TLB 查找（hit 直接得 PA）→ miss 则 walk 多级页表 → 得 PTE → 检查权限/存在位 → page fault 或 PA → L1 cache。TLB 在 MMU 内，缓存 VPN→PPN 映射。

</details>

<details>
<summary>Q3: HFT 减少虚拟内存相关延迟的手段有哪些？</summary>

1) 大页（2MB/1GB）减少 TLB 项；2) CPU 绑定减少上下文切换（避免刷 TLB）；3) mlock 防止页换出；4) 预 fault 热数据（MAP_POPULATE）；5) 对象池避免 malloc。

</details>

<details>
<summary>Q4: malloc、sbrk、mmap 三者的关系？</summary>

malloc 是用户态库函数，管理堆空闲链表。堆不够时调 sbrk/brk（调整 program break）或 mmap（大块分配）向内核申请内存。sbrk 和 mmap 是系统调用，malloc 是封装。

</details>

---

← [§9.11 ←](./section-9.11-C程序常见内存错误.md) · [本章导读](../README.md) · —
