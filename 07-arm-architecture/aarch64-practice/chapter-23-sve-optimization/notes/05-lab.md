# §23.5 实验要点

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Ch23 的 SVE 实验在 QEMU+ARM64 Linux 上完成，包括 RGB 转换、矩阵乘法和字符串操作的 SVE 优化。Pi5 的 Cortex-A76 支持 SVE2。

## 核心要点

### 实验列表

| 实验 | 内容 | 平台 | 关键知识点 |
|------|------|------|-----------|
| 23-1 | RGB24→BGR32（SVE 优化） | SVE | gather/scatter、谓词 |
| 23-2 | 8×8 矩阵乘法运算 | SVE | FMLA 向量化、谓词尾部处理 |
| 23-3 | 用 SVE 优化 strcpy() | SVE | 谓词检测 '\0'、批量操作 |

### 实验环境

> SVE 实验可在 QEMU+ARM64 Linux 上完成。Pi5 的 Cortex-A76 支持 SVE2。

```bash
# QEMU 启动 ARM64 Linux with SVE
qemu-system-aarch64 -M virt -cpu max -smp 2 -m 2G \
    -kernel Image -append "root=/dev/vda" \
    -drive file=rootfs.img,format=raw

# 检查 SVE 支持
cat /proc/cpuinfo | grep Features | grep sve
```

### 实验 23-1 要点

对比 NEON LD3 和 SVE gather 的 RGB→BGR 转换：
- NEON：LD3 交错加载 + ST3 交错存储
- SVE：gather 加载 + 谓词控制 + scatter 存储

### 实验 23-3 要点

SVE strcpy 优化思路与 strcmp 类似：
- 一次加载一个向量的字节
- 用谓词检测 '\0' 位置
- 用 `svst1_scatter_u8index` 选择性存储

## HFT 关联

SVE 实验的价值在于学习谓词驱动的向量化思维。HFT 中类似模式：(1) 批量检查多个订单是否满足条件（谓词过滤）；(2) 批量更新价格表中的多个条目（gather/scatter + 谓词）。当前 HFT 以 NEON 为主，但理解 SVE 有助于在 ARM 服务器升级时快速迁移。QEMU 的 `-cpu max` 可以模拟 SVE，方便在没有 SVE 硬件时开发测试。

## 自测题

1. **如何在 QEMU 中模拟 SVE？Pi5 的 SVE 支持情况如何？**

<details>
<summary>答案</summary>

QEMU 用 `-cpu max` 模拟最大 CPU 特性（包括 SVE/SVE2）。也可以指定向量长度：`-cpu max,sve256=on` 启用 256 位 SVE。Pi5 的 Cortex-A76 **支持 SVE2**，VQ=1（128 位）。在 Pi5 上可以直接运行 SVE 代码，不需要 QEMU 模拟。检查支持：`cat /proc/cpuinfo | grep -o sve2` 或 `lscpu | grep SVE`。注意 QEMU `-cpu max` 可能模拟出真实硬件不具备的特性，生产代码应以目标硬件的实际能力为准。
</details>

2. **实验 23-1 中 SVE 的 gather/scatter 与 NEON 的 LD3/ST3 相比有什么不同？**

<details>
<summary>答案</summary>

NEON 的 LD3/ST3 是**固定模式的交错加载/存储**——硬件自动按 3 路交织模式分离/合并数据，效率高但只支持固定的交织模式（2/3/4 路）。SVE 的 gather/scatter 是**任意模式的非连续访问**——用索引向量指定每个通道的地址，灵活但延迟高（每通道独立访存）。对 RGB→BGR 来说：NEON LD3/ST3 更高效（固定 3 路交织正是 RGB 需要的），SVE gather/scatter 更灵活但性能可能更差。SVE 的优势在非规则访问模式，不是固定交错。
</details>

## 参考与延伸

- [§23.6 精简要点](06-minimal-knowledge.md) — SVE 最小知识集
- [§23.4 strcmp 优化](04-strcmp-optimization.md) — 实验 23-3 的核心算法
