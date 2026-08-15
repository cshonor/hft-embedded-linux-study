# 二 · `unsafe` 关键字的两种核心作用

← [本章目录](./README.md) · 上一节：[01-why-safe-unsafe.md](./01-why-safe-unsafe.md) · 下一节：[03-five-powers.md](./03-five-powers.md)

---

`unsafe` 本质是**责任边界标记**，分为「声明契约」和「履行契约」两类，二者不能混淆。

## 1. 修饰声明：unsafe fn / unsafe trait（立规矩）

作用：向调用者/类型实现者声明一段强制约束，编译器无法自动校验，违规直接产生 UB，责任由外部使用者承担。

### `unsafe fn 函数名()`

代表这个函数存在严格前置条件，调用前必须满足文档约定，否则内存崩溃。

**例子：裸指针写入（声明契约）**

```rust
/// # Safety
/// `ptr` 必须非空、已对齐、指向可写的 `i32`，且调用期间无其它别名写入。
unsafe fn write_i32(ptr: *mut i32, v: i32) {
    *ptr = v;  // 解引用裸指针 — 只能在 unsafe fn 体内或 unsafe 块里
}

fn caller() {
    let mut x = 0;
    let p = &mut x as *mut i32;
    // write_i32(p, 42);  // ❌ 编译错误：调用 unsafe fn 必须包 unsafe 块
    unsafe { write_i32(p, 42); }  // ✅ 调用者承担「p 有效」的责任
}
```

→ 本 crate：[src/five_powers.rs](./src/five_powers.rs) 里的 `increment` 同理。

### `unsafe trait 特征名`

代表实现这个 trait 需要满足编译器无法自动验证的全局安全不变量，实现错误会造成全程序 UB。

**例子：`Send` — 承诺「移到别的线程也安全」**

```rust
struct Packet {
    ptr: *mut u8,
    len: usize,
}

// ❌ 编译器不会自动 impl Send：裸指针默认不 Send
// ✅ 只有当你人工保证「ptr 指向的内存可跨线程访问」时才写：
unsafe impl Send for Packet {}

// 若 ptr 实际指向栈上局部变量，多线程使用 → 数据竞争 → UB
```

标准库里的 `Send`/`Sync` 也是 **unsafe trait**；复合类型字段都满足时编译器**自动**实现，自定义裸指针包装则常需手写 `unsafe impl`（见 ch08 `MyVec`）。

## 2. 修饰执行：unsafe { } / unsafe impl（兑现承诺）

作用：程序员手动确认当前上下文完全满足所有不安全操作的前置契约，主动接管安全责任，告诉编译器「这里我已经校验完毕，允许执行高危操作」。

- `unsafe { ... }` 代码块：包裹 5 种专属不安全操作，没有这个块编译器直接报错。
- `unsafe impl`：配合 unsafe trait 使用，代表你人工核验类型符合该特征的安全要求。

**例子：`unsafe impl` 兑现 trait 契约**

```rust
unsafe trait Marker {}           // 声明：实现者须满足某不变量
unsafe impl Marker for u32 {}    // 履行：我确认 u32 符合要求

fn assert_marker<T: Marker>(x: T) -> T { x }
// assert_marker(0u32);  // ✅ Safe 调用 — trait 约束在编译期检查
```

→ 本 crate：[src/five_powers.rs](./src/five_powers.rs) · `unsafe_trait_demo()`。

## 补充：两个易混案例

### 案例 1 — 内部 unsafe，对外 Safe 接口

函数内部有 unsafe 块，但函数本身**不加** `unsafe fn` → 对外是安全接口。

**例子：`MiniBuf::push`（本 crate）**

```rust
pub fn push(&mut self, byte: u8) {          // 普通 fn — 外部无需 unsafe
    assert!(self.len < self.cap);           // Safe 层：边界检查
    unsafe {
        self.ptr.add(self.len).write(byte); // 内部：ptr::write
    }
    self.len += 1;
}

// 业务侧：
let mut buf = MiniBuf::with_capacity(4);
buf.push(b'A');   // ✅ 全程 Safe
```

与 `Vec::push` 同一模式：作者在模块内完成「cap 足够、ptr 有效、len 一致」等校验，调用者只拿 Safe API。

→ 源码：[src/privacy.rs](./src/privacy.rs) · `cargo run` 末尾会打印 `MiniBuf` demo。

### 案例 2 — 声明即责任

函数标记 `unsafe fn`，内部哪怕一行代码都没有，调用时依然必须加 `unsafe {}`。

```rust
/// # Safety
/// 调用前必须已持有有效的 POSIX 文件描述符。
unsafe fn from_raw_fd(fd: i32) -> FileHandle {
    FileHandle { fd }   // 函数体全是 Safe 代码
}

// let h = from_raw_fd(3);           // ❌ 即使函数体无害，也必须：
let h = unsafe { from_raw_fd(3) };   // ✅ 调用者证明 fd 合法
```

只要声明为不安全函数，调用者就需要手动保证前置条件，**和函数内部逻辑无关**。

→ 五种操作清单：[03-five-powers.md](./03-five-powers.md)
