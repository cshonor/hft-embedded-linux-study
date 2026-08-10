# §22.4 RGB → BGR 转换示例

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

通过 RGB→BGR 颜色空间转换的实际例子，展示 NEON 交错加载（LD3）和交错存储（ST3）如何自动分离和重组数据通道，一次处理 16 像素。

## 核心要点

### 标量 C 版本

```c
void rgb_to_bgr_c(uint8_t *dst, const uint8_t *src, int n) {
    for (int i = 0; i < n; i += 3) {
        dst[i]   = src[i+2];  // B
        dst[i+1] = src[i+1];  // G
        dst[i+2] = src[i];    // R
    }
}
// 每次迭代处理 1 像素（3 字节），需要 3 次读 + 3 次写
```

### NEON 汇编版本

```asm
// 一次处理 16 像素 × 3 = 48 字节
// X0 = src, X1 = dst, X2 = length
rgb_to_bgr_neon:
1:  ld3 {v0.16b, v1.16b, v2.16b}, [x0], #48
    // LD3 交错加载：R→V0, G→V1, B→V2
    // 自动将 R0G0B0R1G1B1... 分离为:
    //   V0 = R0 R1 R2 ... R15
    //   V1 = G0 G1 G2 ... G15
    //   V2 = B0 B1 B2 ... B15

    // 交换 R 和 B（V0 和 V2 的角色）
    // 不需要实际交换寄存器内容，只需改变 ST3 的寄存器顺序

    st3 {v2.16b, v1.16b, v0.16b}, [x1], #48
    // ST3 交错存储：B→V2, G→V1, R→V0
    // 写回为 B0 G0 R0 B1 G1 R1 ...

    subs x2, x2, #48
    b.gt 1b
    ret
```

### NEON intrinsics 版本

```c
#include <arm_neon.h>
void rgb_to_bgr_neon(uint8_t *dst, const uint8_t *src, int n) {
    int i;
    for (i = 0; i + 48 <= n; i += 48) {
        uint8x16x3_t rgb = vld3q_u8(src + i);  // LD3
        // rgb.val[0] = R, rgb.val[1] = G, rgb.val[2] = B

        uint8x16x3_t bgr;
        bgr.val[0] = rgb.val[2];  // B → 位置0
        bgr.val[1] = rgb.val[1];  // G → 位置1
        bgr.val[2] = rgb.val[0];  // R → 位置2

        vst3q_u8(dst + i, bgr);  // ST3
    }
    // 处理尾部
    for (; i < n; i += 3) {
        dst[i] = src[i+2];
        dst[i+1] = src[i+1];
        dst[i+2] = src[i];
    }
}
```

### 对比分析

| 方面 | 标量 C | NEON 汇编 | NEON intrinsics |
|------|--------|-----------|-----------------|
| 每次迭代 | 3 字节（1 像素） | 48 字节（16 像素） | 48 字节（16 像素） |
| 循环次数 | n/3 | n/48 | n/48 |
| 指令数/像素 | ~6 条 | ~0.2 条 | ~0.25 条 |
| 加速比 | 1× | ~16-30× | ~15-25× |
| 可读性 | 最好 | 最差 | 好 |
| 可维护性 | 最好 | 最差 | 好 |
| 尾部处理 | 内置 | 需额外代码 | 内置 |

> **LD3/ST3 自动解交织/交织**：RGB 数据在内存中是 R0G0B0R1G1B1...，LD3 自动分离成 R0R1...→V0、G0G1...→V1、B0B1...→V2。ST3 反向操作：将 V2(B)、V1(G)、V0(R) 交织成 B0G0R0B1G1R1... 写回内存。

### LD3 vs 手动分离对比

```asm
; 不用 LD3 的手动分离（低效）
LD1 V0.16b, [X0]      ; 加载 16 字节: R0G0B0R1G1B1...
; 需要多条 EXT/TBL 指令提取 R/G/B
EXT V1.16b, V0.16b, V0.16b, #1   ; 移位提取
EXT V2.16b, V0.16b, V0.16b, #2
; ... 复杂的通道重排
; 比 LD3 多 5-10 条指令

; LD3 一条指令完成（硬件自动解交织）
LD3 {V0.16b, V1.16b, V2.16b}, [X0]
```

### 尾部处理策略

```c
// 策略1: 标量处理剩余
for (i = 0; i + 48 <= n; i += 48) {
    // NEON 处理 16 像素
}
for (; i < n; i += 3) {
    // 标量处理剩余像素
}

// 策略2: 用更窄的 NEON 处理
if (n - i >= 24) {
    // 用 .8b 处理 8 像素
}
if (n - i >= 3) {
    // 标量处理最后几个
}
```

## HFT 关联

虽然 HFT 不直接做图像处理，但 LD3/ST3 的交错加载思路可用于网络包处理：网络包中多个字段交替排列（如 timestamp + symbol + price + volume 重复），可以用 LD4 自动分离到不同寄存器并行处理。

```c
// HFT 网络包字段解交织
// 内存布局: [ts0 sym0 px0 vol0 ts1 sym1 px1 vol1 ...]
// 用 LD4 自动分离 4 个字段

#include <arm_neon.h>
struct market_msg {
    uint64_t timestamp;
    uint32_t symbol_id;
    int64_t  price;
    int64_t  volume;
};

void hft_parse_messages(const uint8_t *raw, int n_msgs,
                         struct market_msg *out) {
    // raw 中每条消息 32 字节，4 个字段交替
    for (int i = 0; i + 4 <= n_msgs; i += 4) {
        // 加载 4 条消息 × 32 字节 = 128 字节
        uint8x16x4_t fields = vld4q_u8(raw + i * 32);
        // fields.val[0] = timestamps (16 bytes)
        // fields.val[1] = symbol_ids (16 bytes)
        // fields.val[2] = prices (16 bytes)
        // fields.val[3] = volumes (16 bytes)
        // 分离后可并行处理每个字段
    }
}

// HFT checksum 计算（16 路并行）
uint32_t hft_ip_checksum(const uint8_t *data, int len) {
    uint32x4_t sum = vdupq_n_u32(0);
    for (int i = 0; i + 16 <= len; i += 16) {
        uint8x16_t v = vld1q_u8(data + i);
        // 16 字节 → 4×32bit 求和
        uint16x8_t h = vpaddlq_u8(v);    // 16×8 → 8×16
        uint32x4_t w = vpaddlq_u16(h);   // 8×16 → 4×32
        sum = vaddq_u32(sum, w);
    }
    return vaddvq_u32(sum);
}
```

## 自测题

1. **LD3 在 RGB→BGR 转换中解决了什么问题？不用 LD3 行不行？**

<details>
<summary>答案</summary>

LD3 解决了**数据解交织**问题——RGB 数据在内存中交替排列（R0G0B0R1G1B1...），要做 R↔B 交换需要先分离 R 和 B。不用 LD3 的话，需要用 LD1 连续加载 + 多条 TBL/MOV 指令手动提取和重排字节，代码复杂且效率低。LD3 一条指令自动将 3 路交错数据分离到 3 个寄存器，ST3 一条指令自动交织写回。这是 NEON 交错加载指令的核心价值。
</details>

2. **NEON 版本一次处理 48 字节，如果数据长度不是 48 的倍数怎么办？**

<details>
<summary>答案</summary>

需要处理**尾部数据**（remainder）。两种方案：(1) 在主循环后用标量 C 处理剩余字节（简单但需要额外代码）；(2) 用 NEON 的标量模式或按更小的通道数处理尾部。实际代码中通常先检查总长度，主循环处理 `n / 48 * 48` 字节，剩余 `n % 48` 字节用标量处理。书中简化版假设长度是 48 的倍数。Linux 内核中的 NEON 代码（如 crypto）都有完整的尾部处理逻辑。
</details>

3. **ST3 {v2.16b, v1.16b, v0.16b} 中寄存器顺序为什么是 V2,V1,V0？**

<details>
<summary>答案</summary>

因为 ST3 按寄存器列表顺序交织存储。输入是 RGB（V0=R, V1=G, V2=B），输出要 BGR，所以存储顺序改为 V2(B), V1(G), V0(R)——ST3 会按 B0G0R0B1G1R1... 的顺序交织写回内存，正好是 BGR 格式。不需要额外的交换指令——只需改变 ST3 的寄存器列表顺序即可实现通道重排。这是 NEON 交错指令的灵活性体现。
</details>

## 参考与延伸

- [§22.3 常用 NEON 指令](03-neon-instructions.md) — LD3/ST3 指令详情
- [§22.6 NEON 内建函数](06-intrinsics.md) — C 层面实现 RGB→BGR
