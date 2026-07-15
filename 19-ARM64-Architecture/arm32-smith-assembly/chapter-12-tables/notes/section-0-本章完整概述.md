## Ch12 完整概述 · 表（查找表与搜索）

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Tables · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **查表换速度** | 用 **RAM/ROM 空间** 换 **确定性、低延迟** 的函数求值 |
| **缩放寻址** | 把 **Ch5 索引寻址** 用于数组：`[base, index, LSL #n]` |
| **对称压缩** | 正弦表只存 **0°–90°**，靠 **象限变换** 覆盖 360° |
| **有序搜索** | **二分查找** — O(log N) vs 线性 O(N) |

**前置：** [Ch5 Load/Store 寻址](../chapter-05-loads-stores-addressing/notes/section-0-本章完整概述.md) · [Ch7 Q 定点](../chapter-07-integer-logic-arithmetic/notes/section-7-7-fractional.md) · [Ch8 分支/循环](../chapter-08-branches-loops/notes/section-0-本章完整概述.md) · （浮点）Ch9–11 可选

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **动机** | §12.1 | [section-12-1-intro.md](./section-12-1-intro.md) |
| **整数查表 · sin 象限** | §12.2 | [section-12-2-int-lookup.md](./section-12-2-int-lookup.md) |
| **浮点查表 · rsqrt** | §12.3 | [section-12-3-float-lookup.md](./section-12-3-float-lookup.md) |
| **二分查找汇编** | §12.4 | [section-12-4-binary-search.md](./section-12-4-binary-search.md) |
| **练习** | §12.5 | [section-12-5-exercises.md](./section-12-5-exercises.md) |

---

### 三、知识流（口述版）

```
Ch5：基址 + 索引×元素大小 → LDR/LDRH
        ↓
§12.2：DCD 表 · Q31 sin · 象限归约 → 查表 ± 符号
        ↓
§12.3：VLDR + 文字池 · 半精度 rsqrt 估计（3D 光照）
        ↓
§12.4：有序 key 表 · mid=(lo+hi)/2 · ASR #1 · 循环减半区间
        ↓
应用：音频/控制/DSP · 飞控可用 libm 或 C 查表
        ↓
Ch13：查表/搜索常包在 BL 子程序里
```

---

### 四、空间–时间权衡（口述必背）

| 策略 | 优点 | 代价 |
|------|------|------|
| **运行时算 sin/sqrt** | 省 ROM、精度灵活 | 慢、依赖 FPU/库 |
| **全表** | 一次 LDR 即得 | ROM 大 |
| **压缩表（1/4 周期）** | ROM 小、仍很快 | 象限/符号逻辑 |
| **线性搜** | 实现简单、表可无序 | N 大时慢 |
| **二分搜** | log N 次比较 | **必须有序** |

---

### 五、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [Ch5 寻址](../chapter-05-loads-stores-addressing/) | 缩放索引是本章硬件基础 |
| [Ch11 泰勒 sin](../chapter-11-floating-point-data-processing/notes/section-11-8-examples.md) | 同一函数：**算** vs **查** |
| [20 构建](../../20-UBoot-Kernel-Build/) | 链接脚本把 `.rodata` 表放进 Flash |
| [24 飞控](../../24-Motion-Control-Motor/) | 姿态常用 **C libm**；MCU 侧可查表 |

---

### 六、下一章（按 OUTLINE）

→ **[Ch13 栈与子程序](../chapter-13-subroutines-stacks/)**（**精读** — 查表/搜索代码应封装为 `BL` 子程序）
