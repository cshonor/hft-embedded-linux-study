## 3. 每应用 PML4 与 CR3 切换

---

### 一、多应用冲突根因

**Ch19–23：** 所有 ELF **链接同一高半基址** `0xffff800000000000`。

| 单应用 | 多应用同时驻留 |
|--------|----------------|
| **SetupPageMaps → run → Clean** | 第二个应用 **覆盖** 第一個 **同 VA 映射** |
| 无冲突 | **#PF / 数据互踩** |

**症状：** 开 **两个终端各跑 rpn/cube** — **崩溃或结果错乱**。

---

### 二、方案：每应用专属 PML4

```
App A: CR3_A  →  VA 0xffff8000…  →  PA 物理页集合 α
App B: CR3_B  →  VA 0xffff8000…  →  PA 物理页集合 β  (不同!)
Kernel: CR3_k →  identity / 内核高半 (共享)
```

| 机制 | 说明 |
|------|------|
| **独立 PML4 根** | 每 **运行中应用** 一棵 **页表树** |
| **相同 VA 不同 PA** | **链接基址不必改** — **MMU 隔离** |
| **任务切换** | **SwitchContext** 时 **`mov cr3, app_cr3`** |

→ [Ch19 SetupPageMaps](../chapter-19-paging/notes/section-5-高半区链接与SetupPageMaps.md)

---

### 三、加载与切换流程

```cpp
struct Task {
    PageMap* app_pml4;   // 应用专属
    …
};

void LoadApp(Task* t, ELF& elf) {
    t->app_pml4 = AllocatePML4();
    SetupPageMaps(t->app_pml4, elf);
}

void SwitchTo(Task* t) {
    if (t->is_running_app)
        LoadCr3(t->app_pml4);
    else
        LoadCr3(kernel_pml4);
    SwitchContext(&t->ctx);
}
```

**与 Ch19 CleanPageMaps：** **应用退出** 仍 **递归释放** 该 **PML4 子树** — **KillApp/exit** 路径统一。

---

### 四、与 Linux 进程地址空间

| MikanOS Ch24 | Linux |
|--------------|-------|
| **每应用 PML4** | **mm_struct** · **独立页表** |
| **固定链接 VA** | **ASLR** 可选 — 概念同 **per-process 映射** |

→ [CSAPP Ch9 虚拟内存](../../../01-CSAPP-3rd/chapter-09-virtual-memory/)

---

← [2. F2/光标](./section-2-F2新终端与光标独立.md) · 下一节 [4. noterm](./section-4-窗口层级Bug与noterm.md)
