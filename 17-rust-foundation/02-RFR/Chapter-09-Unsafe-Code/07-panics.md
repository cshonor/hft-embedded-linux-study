# 3.3 Panics（Panic 与不变式）

> 所属：**Great Responsibility** · [← 章索引](./README.md)

← [06 有效性](./06-validity.md) · 下一节 [08 转换](./08-casting.md)

前置 → [06 Validity](./06-validity.md) · [03 MaybeUninit](./03-calling-unsafe-functions.md) · [05 UB 与三大契约](./05-what-can-go-wrong.md)

> ER → [Item 16 panic 安全](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md)

---

## 原文完整翻译

### 核心问题

手动维护复杂数据不变式时（典型：**先改 `Vec` 的 `len`，再逐个初始化尾部元素**），若中间调用可能 **panic** 的用户代码，会产生严重内存安全问题：

1. **栈展开**（panic unwind）时，会在**数据不一致状态**下执行 `Drop`；
2. 示例：`len` 已增大，尾部元素尚未初始化；析构器按「全部元素有效」假设释放 → **二次释放**、**读垃圾内存** → UB。

### 三类工程对策

1. **Guard / defer 守卫** — RAII 在 panic 时自动回滚至合法不变式；
2. **`MaybeUninit` 逐元素提交** — 先完成初始化，最后统一 `set_len`，消除不一致窗口；
3. **缩小不一致窗口** — 压缩「元数据已改、元素未就绪」的代码区间。

---

**Panic 本身不是 UB** — 但 panic **栈展开**时若数据结构处于**不一致状态**，`Drop` 可能按错误假设访问内存 → **UB**。

```text
不变式（Invariant）     容器永远应满足的合法约束
不一致窗口             临时打破不变式的一段代码
panic unwind + Drop    在窗口内触发 → 读未初始化 / 双重释放
```

---

## 一、不变式与不一致窗口

### 不变式（Invariant）

自定义容器须**永远**维持的合法约束。以 `Vec<T>` 为例：

**`len` == 缓冲区中已完全初始化且 Valid 的元素个数**（`0..len` 每个槽位都是合法 `T`）。

### 不一致窗口

临时打破不变式的代码区间：

```text
1. vec.set_len(len + 1)   ← 不变式破坏，窗口打开
2. 初始化 vec[len-1]      ← 若此处 panic…
3. （窗口应在此关闭）
```

窗口内一旦 panic → 不变式**永久破损**（直至 `Drop` 读错内存）→ `Vec::drop` 仍信 `len` → 对未初始化尾部执行 `drop` → UB。

---

## 二、Panic unwind 致命链（拆解）

| 步骤 | 动作 |
|------|------|
| 1 | 手动扩容，**`len += 1`**（进入不一致窗口） |
| 2 | 循环构造新元素，某次构造 **`panic!`** |
| 3 | 栈展开 → 调用 **`Vec` 的 `Drop`** |
| 4 | `Drop` 读当前 `len`，对 `0..len` 每个元素 `drop` |
| 5 | 尾部未初始化内存被当作合法 `T` → 读垃圾比特 / 重复释放 → **UB** |

> **关键区分**：panic 本身**有定义**；**不一致状态下的 `Drop`** 才是 UB。

→ 未初始化元素违反 [06 Validity](./06-validity.md)；典型 [05](./05-what-can-go-wrong.md) 静默 UB 场景。

---

## 三、三类工程对策

### 1. Guard / RAII defer 守卫

守卫在 **`Drop` 时回滚**（正常返回或 panic unwind 都会执行）。全部初始化成功后，**提交**新长度（更新 `old_len`，使回滚变为 no-op）：

```rust
struct VecLenGuard<'a> {
    vec: &'a mut Vec<u32>,
    old_len: usize,
}

impl Drop for VecLenGuard<'_> {
    fn drop(&mut self) {
        // SAFETY: old_len 始终是此前合法的 len
        unsafe { self.vec.set_len(self.old_len) };
    }
}

fn push_with_guard(vec: &mut Vec<u32>, value: u32) {
    let mut guard = VecLenGuard {
        vec,
        old_len: vec.len(),
    };
    unsafe {
        guard.vec.set_len(guard.old_len + 1);
    }
    // 若下面 panic，guard::drop 回滚 len
    let last = guard.vec.last_mut().unwrap();
    *last = value;
    guard.old_len = guard.vec.len(); // 提交：Drop 时 set_len 不变
}
```

> 生产代码对照标准库 `Vec` 内部实现；也可用 `ManuallyDrop` / `mem::forget(guard)` 在成功路径放弃回滚。

### 2. `MaybeUninit` 逐元素提交（标准库路线）

**先全部初始化，最后一次性 `set_len`** — 全程无不一致窗口。`Vec::push` / `extend` 底层近似此模式：

| 步骤 | 说明 |
|------|------|
| 1 | 缓冲区用 `[MaybeUninit<T>]`（如 `spare_capacity_mut()`）承载新槽位 |
| 2 | 在 `MaybeUninit` 上 `write` 构造；中途 panic 不污染已提交 `len` |
| 3 | **全部成功**后一次 `set_len` |

→ [03 MaybeUninit](./03-calling-unsafe-functions.md) · Nomicon [05 Uninit](../../04-Rust-Nomicon/05_Uninit_Mem/README.md)

### 3. 缩小不一致窗口

- **不要**在循环前提前 `set_len`  
- 可能 panic 的逻辑**前置**到修改元数据之前  
- 「打破不变式」与「修复不变式」之间只留**极短、无 panic** 的代码  

---

## 四、可运行对比：错误扩容 vs `MaybeUninit` 修复

以下用 `spare_capacity_mut()` 演示标准库路线；**错误版**用 Miri 可观测 UB（勿在生产模仿）。

### ❌ 错误：先 `set_len`，再可 panic 的初始化

```rust
/// 不一致窗口：len 已撒谎，初始化 panic → Drop 读未初始化尾部
fn bad_push_string(vec: &mut Vec<String>, build: impl FnOnce() -> String) {
    let old_len = vec.len();
    if vec.capacity() == old_len {
        vec.reserve(1);
    }
    unsafe {
        vec.set_len(old_len + 1); // 窗口打开
    }
    let slot = unsafe { vec.get_unchecked_mut(old_len) };
    *slot = build(); // build() panic → unwind → Vec::drop 对未初始化槽 drop → UB
}
```

### ✅ 修复：`MaybeUninit` + 最后提交

```rust
use std::mem::MaybeUninit;

fn good_push_string(vec: &mut Vec<String>, build: impl FnOnce() -> String) {
    let old_len = vec.len();
    if vec.capacity() == old_len {
        vec.reserve(1);
    }
    let slot: &mut MaybeUninit<String> = &mut vec.spare_capacity_mut()[0];
    slot.write(build()); // panic 时 len 仍 == old_len，Drop 安全
    unsafe {
        vec.set_len(old_len + 1); // 仅当元素已 Valid 才提交
    }
}
```

```rust
// 演示（Miri 下 bad_push_string 会报 UB）：
// let mut v = vec!["ok".to_string()];
// bad_push_string(&mut v, || panic!("during init"));
// good_push_string(&mut v, || panic!("during init")); // len 不变，安全
```

→ Miri 用法：[12 验证工作](./12-check-your-work.md) · ER [Item 16 demo](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/)

---

## 五、关联知识点与工程规范

| 联动 | 说明 |
|------|------|
| [06 Validity](./06-validity.md) | 未初始化元素 = 无效值；`drop` 读无效值 → UB |
| [05 What Can Go Wrong](./05-what-can-go-wrong.md) | 静默损坏、优化后暴露 |
| [10 管理边界](./10-manage-boundaries.md) | 不一致窗口也须在信任域内用 Guard / `MaybeUninit` |
| ER Item 16 | **异常安全（panic safety）**：正常返回 / panic unwind 均不得脱离合法不变式 |

**常见踩坑**：手改 `Vec` / `Box` 元数据 · 批量分配构造 · 自定义集合 / arena / 内存池

---

## 六、避坑总结

1. **绝不在**「更新 len/元数据」与「元素初始化完成」之间插入可 panic 代码。  
2. 优先 **`MaybeUninit` + 最后 `set_len`**，从根源消除不一致窗口。  
3. 无法避免临时不一致 → **RAII Guard** panic 回滚。  
4. unsafe 审计须查两条：**Validity** + **panic 异常安全**。

---

## 速记

## 关键区分

| | Panic | UB |
|---|-------|-----|
| 定义 | 有（栈展开或 abort） | 无保证 |
| 本节的 UB | — | **不一致状态下 Drop** |

## 不一致窗口

`set_len` 增大 **先于** 尾部初始化 → 中间 panic → `Vec::drop` 读未初始化 → UB

## 三对策

1. **Guard** — panic 时 `set_len(old_len)` 回滚；成功则提交 `old_len`  
2. **MaybeUninit** — `spare_capacity_mut` + `write`，最后 `set_len`（`Vec::push` 路线）  
3. **缩小窗口** — 元数据修改与初始化之间无 panic  

## 可运行对比

- ❌ `set_len` → `build()`（可 panic）  
- ✅ `MaybeUninit::write` → `set_len`  

## 审计两条

- Validity（06）  
- panic safety（本节）

## 自测

- [ ] 为何 panic 本身不是 UB，但本节场景会产生 UB？  
- [ ] `Vec::push` 为何不先 `len+=1` 再构造元素？  
- [ ] Guard「提交」时为何改 `old_len` 即可取消回滚？

