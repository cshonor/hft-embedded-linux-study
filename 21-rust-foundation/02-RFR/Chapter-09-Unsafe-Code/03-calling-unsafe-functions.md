# 2.2 Calling Unsafe Functions（调用 unsafe 函数）

> 所属：**Great Power** · [← 章索引](./README.md)

← [02 裸指针](./02-raw-pointers.md) · 下一节 [04 unsafe trait](./04-unsafe-traits.md)

前置 → [01 unsafe 关键字](./01-unsafe-keyword.md)（`unsafe fn` 与 `unsafe {}`）

---

调用被标记为 **`unsafe fn`** 的函数（含标准库里的 `assume_init`、`ptr::write`、`get_unchecked`、C FFI 等），必须在 **`unsafe {}`** 块中进行 — 与 [01 §二](./01-unsafe-keyword.md) 五类超能力之「调用 `unsafe fn`」对应。

```text
unsafe fn  →  立契约（函数声明）
unsafe { } →  调用方履行契约（证明前提成立）
```

---

## 一、三大典型场景概览

| 场景 | 代表 API | 为何 unsafe |
|------|----------|-------------|
| **FFI** | `extern "C" { fn ... }` | 外部代码不遵守 Rust 不变量 |
| **`_unchecked` 系列** | `slice.get_unchecked` | 跳过边界检查，越界 → UB |
| **未初始化内存** | `MaybeUninit::assume_init` | 须证明内存已是合法 `T` 值 |

---

## 二、FFI — 外部函数接口

### 为何必须 `unsafe`

1. C 等语言**没有**所有权、借用、生命周期 — 裸指针随意读写、越界、释放后复用，编译器不拦。
2. Rust 调用 `extern "C"` 函数时，编译器**无法校验**外部代码是否破坏 Rust 内存安全（悬垂指针、别名违规、缓冲区溢出等）。
3. 因此 Rust 侧调用外部函数**强制** `unsafe` — **安全责任完全在调用者**：入参、内存寿命、调用约定（calling convention）须全部合规。

```rust
extern "C" {
    fn abs(input: i32) -> i32;
}

let n = unsafe {
    // SAFETY: `abs` 为标准 C ABI；`input` 为合法 `i32`
    abs(-42)
};
```

| 要点 | 说明 |
|------|------|
| `extern "C"` | 声明外部符号的 ABI（还有 `extern "system"` 等） |
| 声明 vs 调用 | 声明 `extern` 块本身不一定 unsafe；**调用**须 `unsafe` |
| 安全封装 | 用 Safe 包装函数检查指针/长度后再调 C → [第 11 章](./../Chapter-11-Foreign-Function-Interfaces/README.md) |

→ 进阶 [第 11 章 FFI](../Chapter-11-Foreign-Function-Interfaces/11-外部函数接口-FFI-深度解析.md) · Book [19.1](../../00-Book/19-advanced-features/19.1-不安全Rust.md)

---

## 三、`_unchecked` 系列 — 跳过运行时检查

### 安全红线

| API | 行为 | 违反后果 |
|-----|------|----------|
| `slice.get(idx)` | 运行时边界检查 | 越界 → **panic**（Safe） |
| `slice.get_unchecked(idx)` | 直接偏移取值，**无检查** | 越界 → **UB** |

**规范**：

- **禁止为偷懒随便用**；
- 仅用于**热点循环**、可量化的性能瓶颈；
- 必须有**前置逻辑**数学上保证下标合法；
- 块前写清 `// SAFETY:` 依据。

```rust
let arr = [1, 2, 3, 4];

for i in 0..arr.len() {
    let v = unsafe {
        // SAFETY: `i` 来自 `0..arr.len()`，恒有 `i < arr.len()`
        *arr.get_unchecked(i)
    };
    println!("{v}");
}
```

### 其它常见 `_unchecked`

| API | 跳过什么 |
|-----|----------|
| `get_unchecked` / `get_unchecked_mut` | 切片边界 |
| `str::from_utf8_unchecked` | UTF-8 有效性（须已验证） |
| `Vec::set_len` | 长度与已初始化元素一致性 |

**TOCTOU 陷阱**：多线程或检查后、使用前长度被改 → `get_unchecked` 仍可能 UB → Nomicon [07 数据竞争](../../04-Rust-Nomicon/07_Concurrency_Atomic/01-data-races.md)

---

## 四、`MaybeUninit<T>` — 未初始化内存

### 核心作用

Rust 默认要求变量**完全初始化**。`MaybeUninit<T>` 合法标记「这块内存**可能**还没有合法 `T`」：

| 场景 | 说明 |
|------|------|
| 大数组栈分配 | 避免先零初始化再覆盖的开销 |
| FFI 缓冲区 | 由外部填充，再 `assume_init` |
| 自定义容器 / 内存池 | `Vec` 扩容内部类似模式 |

### 关键 API

| 方法 | Safe? | 含义 |
|------|:-----:|------|
| `MaybeUninit::uninit()` | ✅ | 分配未初始化槽位 |
| `.write(val)` | ✅ | 写入并初始化 |
| `.assume_init()` | ❌ unsafe | 断言已初始化，取出 `T` |
| `.assume_init_read()` | ❌ unsafe | 读出 `T`（可能复制） |

**`assume_init` 义务**：调用前必须**完整写入合法 `T` 值**（满足 **validity**）；读未初始化内存 → UB（脏数据、崩溃）。

```rust
use std::mem::MaybeUninit;

let mut buf = MaybeUninit::<u32>::uninit();
buf.write(100);

let val = unsafe {
    // SAFETY: `write(100)` 已完整初始化该槽位
    buf.assume_init()
};
assert_eq!(val, 100);
```

| 注意 | 说明 |
|------|------|
| Drop `MaybeUninit` | **不会** drop 内部（可能无合法值） |
| 勿用 `mem::uninitialized` | 已废弃；新代码**永远**用 `MaybeUninit` |

→ Nomicon [05 未初始化内存](../../04-Rust-Nomicon/05_Uninit_Mem/README.md) · [03-unchecked](../../04-Rust-Nomicon/05_Uninit_Mem/03-unchecked.md) · validity [06](./06-validity.md)

---

## 五、三类场景可运行对比

```rust
use std::mem::MaybeUninit;

fn main() {
    demo_get_vs_unchecked();
    demo_maybe_uninit();
    demo_ffi_abs();
}

/// 1. Safe `get` vs unsafe `get_unchecked`
fn demo_get_vs_unchecked() {
    let arr = [10, 20, 30];

    for i in 0..arr.len() {
        // Safe：越界 panic
        let safe = arr.get(i).copied().unwrap();

        // unsafe：须自行保证 i < len
        let fast = unsafe {
            // SAFETY: `i` 来自 `0..arr.len()`
            *arr.get_unchecked(i)
        };

        assert_eq!(safe, fast);
    }

    // arr.get(99)              // panic
    // unsafe { arr.get_unchecked(99) }  // UB — 勿运行
}

/// 2. MaybeUninit：先 write，再 assume_init
fn demo_maybe_uninit() {
    let mut slot = MaybeUninit::<String>::uninit();
    slot.write(String::from("hello"));

    let s = unsafe {
        // SAFETY: `write` 已初始化
        slot.assume_init()
    };
    assert_eq!(s, "hello");
}

/// 3. FFI：调用 C 标准库函数（平台需能链接 `abs`）
extern "C" {
    fn abs(input: i32) -> i32;
}

fn demo_ffi_abs() {
    let n = unsafe {
        // SAFETY: 标准 C `abs`，参数为合法 i32
        abs(-42)
    };
    assert_eq!(n, 42);
}
```

---

## 六、整体调用准则

| # | 准则 |
|:-:|------|
| 1 | `unsafe` **不是**关掉借用检查 — 是**安全责任移交**；编译器不校验的部分由你证明 |
| 2 | **能不用就不用** — 优先 Safe 标准库 API |
| 3 | 每个 `unsafe` 块写 **`// SAFETY:`** — 说明为何安全、满足哪些前置约束 → [11 文档](./11-documentation.md) |
| 4 | FFI / `assume_init` / `_unchecked` 各自有**独立**契约，不可混为一谈 |
| 5 | 封装：把 `unsafe` 缩进私有模块，对外只暴露 Safe API → [10 管理边界](./10-manage-boundaries.md) |

---

## 七、常见误区

| 误区 | 纠正 |
|------|------|
| 声明 `extern "C"` 就要写 unsafe | **调用**外部函数才须 `unsafe` |
| `get_unchecked` 只是少一次 panic | 越界是 **UB**，不是 panic |
| `MaybeUninit::uninit()` 可以随便读 | 读前必须 `write` 或 `ptr::write` 等完整初始化 |
| `assume_init` 会帮你检查 | **不会** — 完全信任你的断言 |
| 函数标记了 `unsafe fn` 就随便调 | 每次调用仍须自己的 `unsafe {}` + 证明 |

---

## 八、口诀

- **FFI**：外部不守 Rust 规矩，调用你兜底。  
- **`_unchecked`**：证明下标 / 不变量，热点才用。  
- **`MaybeUninit`**：`write` 初始化，`assume_init` 才取出。  
- **通用**：`// SAFETY:` 写清楚，能封装就封装。

---

## 自测

- [ ] 说明为何 `extern "C"` 调用须 `unsafe`，与裸指针解引用的 unsafe 有何不同  
- [ ] `get(99)` 与 `get_unchecked(99)` 后果差在哪？  
- [ ] `assume_init` 前漏掉 `write` 会怎样？  
- [ ] 举一个适合 `get_unchecked` 的循环条件

→ 下一节：[04 unsafe trait](./04-unsafe-traits.md)
