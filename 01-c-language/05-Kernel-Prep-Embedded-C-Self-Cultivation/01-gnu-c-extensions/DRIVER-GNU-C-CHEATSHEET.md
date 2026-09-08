# 驱动开发必会 GNU C 扩展 · 速查表

> 跟读内核/驱动用 · 全文策略见 [6.0](./6.0-driver-how-much-gnu-c.md) · 分节精读见本目录 6.2–6.7 等

| 扩展 | 典型写法 | 驱动里干什么 | 深挖 |
|------|----------|--------------|------|
| **指定初始化** | `{ .compatible = "a,b" }, { .probe = fn }` | `of_match_table`、`file_operations`、`platform_driver` | [6.2](./6.2-designated-init/) · [struct 驱动向](../../01-Primer-K-and-R-C/ch06-structures/6.0-struct-for-drivers.md) |
| **`packed`** | `__attribute__((packed))` | 寄存器/协议结构体紧凑布局，忌乱对齐 | [6.7.4](./6.7-aligned/6.7.4-属性声明-packed.md) |
| **`section`** | `__attribute__((section("…")))` | 放到指定 ELF 段（init、特殊表） | [6.6](./6.6-section/) |
| **`unused` 等** | `__attribute__((unused))` | 消警告、标记 | 属性总览见 ch06 |
| **零长/柔性数组** | `char data[0];` / `char data[];` | 变长缓冲尾随 | [6.5](./6.5-zero-length-array/) |
| **语句表达式** | `({ …; expr; })` | 带类型安全的宏 | [6.3](./6.3-statement-expr/) |
| **`typeof`** | `typeof(x)` | `min`/`max`、类型推导宏 | [6.4](./6.4-typeof-container-of/) |
| **`container_of`** | （基于 typeof + offsetof） | 从成员指针还原结构体 | [6.4.3](./6.4-typeof-container-of/6.4.3-Linux内核中的container_of宏.md) |
| **`asm`** | `asm volatile (…)` | 架构相关、屏障、原子等 | 按需；demo 见 [demo07](./demo/) |
| **`likely`/`unlikely`** | `__builtin_expect` | 分支预测提示 | [6.11.6](./6.11-builtin/6.11.6-Linux内核中的likely和unlikely.md) |

### 指定初始化（读驱动第一眼）

```c
static const struct of_device_id match[] = {
	{ .compatible = "vendor,device" },
	{ /* sentinel */ }
};
```

### `packed`（寄存器映射）

```c
struct regmap {
	u32 ctrl;
	u32 status;
} __attribute__((packed));
```

### 语句表达式 + typeof（宏）

```c
#define min(a, b) ({ \
	typeof(a) _a = (a); \
	typeof(b) _b = (b); \
	_a < _b ? _a : _b; \
})
```

---

**优先级：** 标准 C → 本表左列 → 其余 GNU 扩展现查。  
**红线：** 内核驱动不用 C++；不用用户态 libc 套路。

---

## 代码自测

**题目 1：** 以下 GNU C 扩展在内核中的典型用途是什么？
```c
container_of(ptr, type, member)   // ?
__attribute__((section(".init")))    // ?
__attribute__((aligned(64)))         // ?
__attribute__((packed))              // ?
__builtin_expect(exp, c)             // ?
typeof(x)                            // ?
```
<details>
<summary>参考答案</summary>

- `container_of`：从结构体成员指针反推结构体指针，泛型链表核心
- `section(".init")`：将函数放入 .init 段，启动后释放节省内存
- `aligned(64)`：强制 Cache Line 对齐（64B），防止 false sharing
- `packed`：取消结构体填充，映射网络协议头/硬件寄存器
- `__builtin_expect`：分支预测提示，`likely()`/`unlikely()` 底层
- `typeof`：类型推导，宏中声明与参数同类型的变量

这些扩展是内核代码的"基础词汇"，不掌握无法读懂内核。

</details>
