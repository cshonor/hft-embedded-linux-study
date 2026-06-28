## 4. Local APIC 定时器测量

> **「先测量、后优化」** — 不凭感觉改代码。

---

### 一、问题现象

图层合成后 **逻辑正确**，但：

| 现象 | 用户感受 |
|------|----------|
| 每次鼠标移动 **全屏重绘** | **卡顿** · **闪烁** |
| 单点 `WritePixel` + 格式转换 **× 百万次** | CPU **爆炸** |

**需要：** 可重复的 **耗时数字** — 而非「感觉很慢」。

---

### 二、Local APIC Timer

**Local APIC** — 每 CPU 核心 **本地中断控制器**（与 Ch7 **MSI** 同属现代 x86 中断体系）。

**APIC 定时器** — 递减计数器，可配置 **分频** 与 **初始计数值**：

```cpp
StartMeasure();
LayerManager::DrawAll();   // 或仅光标相关重绘
EndMeasure();              // 读 APIC Current Count 差值
```

| 书中基准（示意） | 含义 |
|------------------|------|
| **~2.5×10⁸ tick** | **一次鼠标移动触发的全屏重绘** 极长 |

**价值：**

- 优化 **Shadow Buffer** 后 **同一脚本对比** — 证明 **~67×** 提升
- 与 [03 SysPerf](../../../03-Systems-Performance-2nd/) **基准测试** 思想一致

→ **Ch11** 进一步用 APIC/ACPI **系统 tick** — 本章先作 **profiler**

---

### 三、测量习惯

```
1. 固定场景（分辨率、层数、移动 1 像素）
2. APIC 计时包围 DrawAll()
3. 记录 baseline
4. 改 Shadow/memcpy
5. 再测 — 用 **比值** 说话
```

---

← [3. LayerManager](./section-3-Window与LayerManager.md) · 下一节 [5. Shadow Buffer](./section-5-阴影缓冲区与memcpy加速.md)
