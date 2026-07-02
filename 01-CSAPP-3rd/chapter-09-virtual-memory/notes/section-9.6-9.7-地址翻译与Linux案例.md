## 9.6–9.7 地址翻译、TLB 与 Linux 案例

### 9.6 地址翻译

**VA 划分（概念）：** `VPN | VPO` → 经页表得 `PPN | PPO`

#### 9.6.1 结合 Cache 与 VM

- **物理寻址 cache (PA)** vs **虚拟寻址 cache (VA)** — 现代 x86 常用 **物理索引物理标记 (PIPT)** L2/L3，避免别名
- **缺页或权限检查** 在 cache 访问路径上

#### 9.6.2 TLB

- **页表在内存** — 每次翻译读 PTE 太慢
- **TLB** — 页表缓存（MMU 内），**全相联/组相联**，典型 64–1024 项
- **TLB miss** → 页表 walk（可能多级）— 数十周期

**大页 (2MB/1GB)：** 同样工作集 **更少 TLB 项** — HFT 关键优化

#### 9.6.3 多级页表

- 单级页表 4KB 页 × 48 位 VA → 表太大
- **四级页表** (x86-64) — 未用区域不占物理页

#### 9.6.4 端到端翻译

```
VA → TLB hit? → PA → L1 → ... → 或 TLB miss → 页表 walk → 可能 page fault
```

### 9.7 Intel Core i7 / Linux 案例（9.7.1–9.7.2）

- **4 级页表** + **PCID**（进程上下文 ID，减 TLB flush）
- **Linux：** 每进程 `mm_struct`、**VMA** 链表、`/proc/pid/maps`
- **透明大页 THP** — 内核自动 4K 合并 2M；**延迟敏感** 常 **显式 hugepage** 或关 THP（→ [note-THP](../../../05-Linux-Virtual-Memory-Manager/chapter-03-page-table-management/notes/note-透明大页THP.md)）

**HFT 检查清单：**

```bash
grep -i huge /proc/meminfo
numastat -p <pid>
cat /proc/<pid>/smaps_rollup | head
```

- **`mbind`/`set_mempolicy`** — NUMA 本地分配（→ [13-Systems-Performance Ch7](../../../13-Systems-Performance-2nd/chapter-07-memory/)）

---

← [本章导读](../README.md)
