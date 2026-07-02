## 5. 写入时复制与 invlpg

---

### 一、问题：多开相同应用

**Ch24 noterm apps/cube × N：**

| 无 CoW | 每个实例 **完整拷贝 ELF LOAD** |
|--------|-------------------------------|
| **.text 相同** | **重复物理帧** — 浪费 |

---

### 二、CoW 策略

**加载时（或 fork 式共享 — 本书 **同 ELF 多实例**）：**

```
Task A PML4: VA 0xffff8000+text → PFN 100 (shared)
Task B PML4: VA 0xffff8000+text → PFN 100 (shared)
PTE flags: Present | User          // Writable=0 只读
```

**.data 初始可共享只读** — **首次写全局变量：**

```
#PF — Write to readonly page
HandleCoW(va):
    new_frame = AllocateFrame()
    memcpy(new_frame, old_frame, 4096)
    RemapPTE(task, va, new_frame, Present|User|Writable)
    invlpg(va)
```

| 步骤 | 说明 |
|------|------|
| **共享** | 多 PML4 **同 PFN** · **R/W=0** |
| **写 fault** | **复制帧** · **仅本任务 PTE 改可写** |
| **其他任务** | 仍 **共享旧帧**（若未写） |

→ [05 LKD 内存管理](../../../03-Linux-Kernel-Development/) · [CSAPP Ch9](../../../01-CSAPP-3rd/chapter-09-virtual-memory/)

---

### 三、invlpg 与 TLB

**改 PTE 后 CPU TLB 可能仍缓存旧 **VA→PA**：

```nasm
invlpg [va]    ; 使该页 TLB 项失效
```

| 不 invlpg | 可能 **写错物理页** · **CoW 后仍读共享** |
|-----------|------------------------------------------|
| **切换 CR3** | 全 TLB flush — 任务切换已部分覆盖 |

**书中强调：** **CoW 正确性** 依赖 **页表 + TLB 一致**。

→ [Ch19 TLB 提及](../chapter-19-paging/notes/section-4-x86-64四级分页.md)

---

### 四、与 Demand 分工

| 机制 | 适用 |
|------|------|
| **Demand** | **匿名堆** · **未映射 VA** |
| **CoW** | **共享 .text/.rodata** · **写 .data** |
| **MapFile** | **文件 backed 页** |

---

← [4. memstat](./section-4-memstat与位图统计.md) · 下一节 [6. 小结](./section-6-小结与后续.md)
