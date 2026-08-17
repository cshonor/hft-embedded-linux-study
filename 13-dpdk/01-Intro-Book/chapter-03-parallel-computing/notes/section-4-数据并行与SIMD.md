## 4. 数据并行与 SIMD

> **单指令多数据** — 拓宽位宽，一条指令处理多个元素

---

### 一、SIMD 基础

| 寄存器 | 指令集 | 宽度 | 元素容量 (uint32) | 平台 |
|--------|--------|:---:|:---:|------|
| **XMM** | SSE/SSE2-SSE4.2 | **128 bit** | 4 | 所有 x86-64 |
| **YMM** | AVX/AVX2 | **256 bit** | 8 | Sandy Bridge+ (2011) |
| **ZMM** | AVX-512 | **512 bit** | 16 | Skylake-X+ (2017) |
| **V0-V31** | NEON/SVE | **128/2048 bit** | 4-16 | ARM |

即使 **单条指令** 不再拆分并发，仍可通过 **数据并行** 提高 **有效吞吐**。

 [15-computer-architecture Ch4 SIMD/GPU](../../../../15-computer-architecture/chapter-04-vector-simd-gpu/) · [Ch1 SIMD 提及](../../chapter-01-dpdk-intro/notes/section-3-性能最佳实践.md)

---

### 二、I/O 密集负载的收益

DPDK 类负载：**访存带宽** 常是瓶颈。

SIMD 的 **直接好处：**

- **最大化 L1 带宽** — 宽 Load/Store 一次搬更多字节
  - SSE: 16B/cycle → 2 cycle 搬一个 cache line
  - AVX2: 32B/cycle → 1 cycle 搬半个 cache line
  - AVX-512: 64B/cycle → 1 cycle 搬一整个 cache line
- 减少流水线因 **等待内存** 而 **stall**
- 减少 **指令数量** — 1 条 AVX2 指令 = 8 条标量指令

---

### 三、DPDK 中的 SIMD 实战

**1. `rte_memcpy` — DPDK 专用高速拷贝：**

DPDK **放弃** libc `memcpy`，专用 **`rte_memcpy`**：

| 技巧 | 说明 |
|------|------|
| **最宽 Load/Store** | 平台允许的最大宽度（如 AVX2 **256b**） |
| **Store 地址对齐** | 优先保证 **写** 对齐 — 避免跨 cache line 写 |
| **双 Load / 周期** | 利用超标量 **每周期两条 Load** — 弥补 **非对齐 Load** 损失 |

```c
/* rte_memcpy 内部（AVX2 版本简化） */
/* 一次拷贝 64 字节 — 2× AVX2 Load + 2× AVX2 Store */
static __rte_always_inline void *
rte_memcpy_generic(void *dst, const void *src, size_t n)
{
    __m256i ymm0, ymm1;

    /* 主循环 — 每次 64 字节 */
    while (n >= 64) {
        ymm0 = _mm256_loadu_si256((const __m256i *)src);      /* Load 32B */
        ymm1 = _mm256_loadu_si256((const __m256i *)src + 1);  /* Load 32B */
        _mm256_storeu_si256((__m256i *)dst, ymm0);             /* Store 32B */
        _mm256_storeu_si256((__m256i *)dst + 1, ymm1);         /* Store 32B */
        src += 64; dst += 64; n -= 64;
    }
    /* 尾部处理... */
}
```

**2. CRC32 — SSE4.2 硬件加速：**

```c
/* DPDK rte_hash_crc 使用 SSE4.2 CRC32 指令 */
/* 单周期完成 64-bit CRC — 比软件查表快 10x+ */
#include <rte_hash_crc.h>

uint32_t key = rte_hash_crc(payload, len, initval);

/* 底层映射 */
static inline uint32_t
crc32c_sse42_u64(uint64_t data, uint32_t init_val)
{
    __asm__ volatile(
        "crc32q %[data], %[init_val]"
        : [init_val] "+r" (init_val)
        : [data] "rm" (data)
    );
    return init_val;
}
```

**3. 批量包头解析 — SIMD 并行处理多个包头：**

```c
/* AVX2 并行解析 8 个 IPv4 头的协议字段 */
__m256i proto_mask = _mm256_set1_epi32(0xFF0000);  /* L4 protocol 位置 */
__m256i protos = _mm256_and_si256(
    _mm256_loadu_si256((const __m256i *)packets),  /* 8 个包头 */
    proto_mask
);
/* 一次提取 8 个包的 L4 协议 — 用于分流 */
```

> **深潜：** `lib/librte_eal/*/include/generic/rte_memcpy.h` — 运行时 CPU flag 分派（SSE/AVX/AVX2/AVX-512/NEON）。

**HFT：** 热路径 **避免 memcpy** 优于 **更快 memcpy** — 零拷贝、指针传递优先。但某些场景（如 orderbook snapshot 复制）无法避免时，`rte_memcpy` 是正确选择。

---

### 四、SIMD 使用注意事项

| 注意点 | 说明 |
|--------|------|
| **AVX-512 降频** | Skylake-X 使用 AVX-512 会触发频率降低（license-based frequency） |
| **AVX↔SSE 切换惩罚** | 混用 SSE 和 AVX 指令需 `vzeroupper` — 否则 ~70 cycle 惩罚 |
| **对齐要求** | AVX 对齐 Load 需要 32B 对齐；非对齐版 (`loadu`) 略慢 |
| **编译器向量化** | `-O3 -mavx2` 可自动向量化简单循环，但 DPDK 手写通常更优 |

---

← [3. ILP](./section-3-指令级并发.md) · 下一节 [5. 小结](./section-5-小结与索引.md)
