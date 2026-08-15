## 3.9 结构、联合与对齐

### 3.9.1 结构 (struct)

- 成员按声明顺序分配；**偏移由对齐规则决定**
- 访问 `p->field` → 基址 + 编译期常量偏移 — 单条 `mov` 带偏移

```c
struct Ex {
    char c;    // offset 0
    // 3 bytes padding
    int i;     // offset 4
    double d;  // offset 8
};  // sizeof 可能 16，不是 1+4+8
```

### 3.9.2 联合 (union)

- 所有成员 **共享同一起始地址** — 同一时刻只解释一种类型
- 用途：类型双关、协议变体、位级视图（谨慎 strict aliasing）

### 3.9.3 数据对齐

- **x86-64 原则：** `K` 字节基本类型地址应是 `K` 的倍数
- 编译器插入 **padding**；`#pragma pack` / `alignas` 可改（跨模块 ABI 风险）

**HFT 必读：**

| 主题 | 实践 |
|------|------|
| **false sharing** | 两线程改同一 cache line 不同字段 — 用 `alignas(64)` 隔离热字段 |
| **协议 struct** | 显式 `packed` + 固定宽度类型；**禁止**跨语言裸 `sizeof(struct)` 上网线 |
| **热冷分离** | 把常改字段放同一行、只读元数据另 struct |

```c
// 示意：避免伪共享
alignas(64) struct { atomic<int> seq; } producer;
alignas(64) struct { atomic<int> seq; } consumer;
```

→ [Ch 12 并发](../../chapter-12-concurrent-programming/) · [16-Systems-Performance Ch 6](../../../14-systems-performance/chapter-06-cpus/)

### 自测题

<details>
<summary>1. 结构体 `struct { char c; int i; }` 的 sizeof 是多少？为什么不是 5？</summary>

**8 字节**（64 位系统）。`char c` 占 1 字节，但 `int i` 需要 4 字节对齐，所以 c 后面有 3 字节 padding。结构体大小必须是其最大成员对齐的整数倍。

**HFT 优化**：按大小降序排列成员可减少 padding：`struct { int i; char c; }` sizeof 仍是 8，但 `struct { int i; char c; char d; short s; }` = 8 比 `struct { char c; short s; int i; char d; }` = 12 更紧凑。

</details>

<details>
<summary>2. `union` 和 `struct` 的区别？HFT 中联合体有什么用途？</summary>

`struct` 成员**各自占用独立内存**（偏移不同）。`union` 所有成员**共享同一块内存**（偏移 0），大小 = 最大成员大小。同时只有一个成员有效。

**HFT 用途**：1. 类型双关（type punning）—— `union { uint32_t u; float f; }` 安全地在 int 和 float 间转换（避免 strict aliasing UB）
2. 协议解析——同一区域按不同 layout 解读
3. 寄存器映射——32 位寄存器可按 byte/word/dword 访问

</details>


---

← [本章导读](../README.md)
