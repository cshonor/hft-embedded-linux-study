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

→ [Ch 12 并发](../../chapter-12-concurrent-programming/) · [02-SysPerf Ch 6](../../../02-Systems-Performance-2nd/chapter-06-cpus/)

---

← [本章导读](../README.md)
