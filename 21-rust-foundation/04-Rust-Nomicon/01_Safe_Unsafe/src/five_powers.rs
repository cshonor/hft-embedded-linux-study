//! Nomicon ch1：Unsafe Rust 的五种额外能力。

/// 1 + 2：解引用 raw pointer，并调用 `unsafe fn`。
pub fn raw_pointer_and_unsafe_fn() -> i32 {
    let mut n = 42;
    let ptr = &mut n as *mut i32;
    unsafe {
        increment(ptr);
        *ptr
    }
}

/// `unsafe fn` 声明：调用者必须保证 `ptr` 有效且可写。
unsafe fn increment(ptr: *mut i32) {
    *ptr += 1;
}

/// 3：`unsafe trait` + `unsafe impl`（此处 `u32` 天生可 Send）。
pub fn unsafe_trait_demo() -> u32 {
    assert_send(0u32);
    0
}

unsafe trait Marker {}

unsafe impl Marker for u32 {}

fn assert_send<T: Marker>(value: T) -> T {
    value
}

/// 4：读写 `static mut`（仅在 `unsafe` 块内）。
static mut COUNTER: u32 = 0;

pub fn static_mut_demo() -> u32 {
    unsafe {
        COUNTER += 1;
        COUNTER
    }
}

/// 5：读取 `union` 字段（必须 `unsafe`）。
#[allow(dead_code)]
union IntOrFloat {
    i: u32,
    f: f32,
}

pub fn union_field_demo() -> u32 {
    let u = IntOrFloat { i: 0x3f800000 };
    unsafe { u.i }
}
