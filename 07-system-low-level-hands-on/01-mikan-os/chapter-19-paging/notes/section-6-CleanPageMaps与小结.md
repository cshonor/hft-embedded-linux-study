## 6. CleanPageMaps 与小结

---

### 一、为何必须清理

**SetupPageMaps 分配：**

| 资源 | 若不释放 |
|------|----------|
| **4KiB 数据页** | **物理内存泄漏** |
| **PML4/PDP/PD/PT 表页** | 表项堆积 · **allocator 耗尽** |

**应用返回后** 内核恢复 **kernel CR3** — **应用页表** 可整棵拆除。

---

### 二、CleanPageMaps() 递归

```cpp
void CleanPageMaps(PageMap* pml4) {
    // 遍历已建子树
    // 对每个 PT 项: FreePage(phys_data)
    // 对每个中间表: 递归 FreePage(table)
    // 最后 FreePage(pml4)
}
```

| 原则 | 说明 |
|------|------|
| **仅释放本应用 CR3 子树** | **勿碰** 内核 **identity map** |
| **顺序** | 先 **叶子数据页** · 再 **上级表页** |
| **TLB** | 若需再次运行 **新应用** — 已 **CR3 切换** 即可 |

---

### 三、验证：rpn 成功

```
> rpn 2 3 +
5
exit code: 0
```

| 修复链 | 效果 |
|--------|------|
| **高半链接基址** | 代码/全局 **VA 正确** |
| **SetupPageMaps** | **VA→PA** 与 **LOAD 段** 一致 |
| **CleanPageMaps** | 多次运行 **不泄漏** |

**标志：** MikanOS **分页实战** 成功 — 应用 **按链接器期望的 VA 运行**。

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **根因** | **image-base 0** ≠ 实际加载 |
| **方案** | **虚拟地址 + MMU** |
| **机制** | **四级页表 · CR3** |
| **工程** | **Setup/Clean** · **large code model** |

```
Ch19 应用页表
    ↓
Ch20 syscall + 用户态位 (U/S)
Ch21+ GUI 应用独立地址空间
```

---

### 五、后续索引

| Ch19 主题 | 继续读 |
|----------|--------|
| 系统调用 / 用户态 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |
| 内核分页基础 | [chapter-08-memory](../chapter-08-memory/) 🔴 |
| ELF 加载 | [chapter-18-apps](../chapter-18-apps/) |

---

← [5. SetupPageMaps](./section-5-高半区链接与SetupPageMaps.md) · [Ch 18](../chapter-18-apps/) · [Ch 19 导读](../README.md)
