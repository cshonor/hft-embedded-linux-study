# 3.10 `MaybeUninit`

> 章索引：[第 3 章](./README.md) · 前：[3.9 泄漏与循环引用](./3.9-leaks-and-cycles.md)

---

## 一句话

**`MaybeUninit<T>`** 表示 **「已分配、尚未保证是合法 `T`」的内存** — 标准库用它实现 **`Vec` 扩容、channel 槽位、read 缓冲区**；HFT 里 **预分配未初始化内存池** 几乎绕不开这一节。

---

## 为何不用「全 zero」或 `mem::uninitialized`

| 做法 | 问题 |
|------|------|
| **`vec![0; n]` 再覆写** | 对大单类型 **多余初始化成本** |
| **旧 `uninitialized`** | 已移除；未写入就 read → **UB** |
| **`MaybeUninit<T>`** | 显式状态机：**未 init → write → assume_init / read** |

---

## 核心 API（`std::mem::MaybeUninit`）

```rust
use std::mem::MaybeUninit;

// 栈上未初始化槽位
let mut slot = MaybeUninit::<Quote>::uninit();
// 写入后
unsafe { slot.as_mut_ptr().write(quote); }
let q = unsafe { slot.assume_init() };

// 切片：未初始化堆/池
let buf: &mut [MaybeUninit<Quote>] = ...;
// 逐槽 write，仅对已 init 的做 assume_init_read
```

| 操作 | 含义 |
|------|------|
| `uninit()` | 创建未初始化值 |
| `write(val)` | 原位构造 `T` |
| `assume_init()` | **unsafe** — 断言已 init，拿走 `T` |
| `assume_init_read()` | **unsafe** — 已 init，按值读出 |
| `as_mut_ptr()` | 得 `*mut T` 给 `ptr::write` |

---

## HFT：预分配未初始化内存池

```text
启动时
  allocate pool[N] of MaybeUninit<Event>   // 不调用 N 次 Event::default
热路径
  pop free index → ptr::write / write → 使用 Event
  处理完 → drop_in_place（若需）→ 槽位回收到 free list（仍 MaybeUninit）
```

| 要点 | 说明 |
|------|------|
| **避免重复构造** | 行情/订单 struct 大时，省掉每次 `Default` |
| **与 `Vec` 扩容同源** | `Vec` 对新 capacity **只分配，不默认构造** 新槽（对 `T` 通用） |
| **unsafe 边界** | 你必须保证：**read/assume_init 前必定 write**；释放时用 **`drop_in_place`** |
| **与 3.1 对齐** | 池按缓存行 / 对齐分配，减少 false sharing |

→ [Nomicon 05 Uninit Mem](../../04-Rust-Nomicon/05_Uninit_Mem/README.md) · [RFR Ch09 · 03 calling unsafe](../../02-RFR/Chapter-09-Unsafe-Code/03-calling-unsafe-functions.md) · [06 validity](../../02-RFR/Chapter-09-Unsafe-Code/06-validity.md)

---

## 标准库中的出现位置

- **`Vec::reserve`** — 新内存为 `MaybeUninit` 语义（实现细节见 `alloc`）。
- **`Read::read`** — 读入 `&mut [u8]` 或 `MaybeUninit` 缓冲区。
- **自定义 channel / 无锁队列** — 槽位生命周期常建模为 `MaybeUninit`。

---

## 相关

- [3.1 内存布局](./3.1-layout-and-alignment.md)
- [3.3 UnsafeCell](./3.3-unsafecell.md) — 另一路径绕过 `&T` 不变性
- [第 4 章 容器（规划）](../README.md#目录) — `Vec` 扩容实现对照
