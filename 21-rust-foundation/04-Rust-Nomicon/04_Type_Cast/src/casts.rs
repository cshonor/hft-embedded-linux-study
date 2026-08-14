//! `as` 显式 cast：数字、指针、非传递性。

pub fn numeric_cast() -> u8 {
    let big: i32 = 300;
    big as u8 // 300 mod 256 = 44
}

pub fn pointer_cast_chain() -> usize {
    let x = 42_i32;
    let p: *const i32 = &x;
    // 链式 cast 合法：*const i32 → *const () → *const f64
    let q = p as *const () as *const f64;
    q as usize
}

pub fn demo_non_transitivity_note() -> &'static str {
    // 直接 `p as *const f64` 在多数平台编译失败；
    // 说明 `as` 不具备传递性：e as U1 as U2 ≠ e as U2。
    "see 00-overview.md §3 and pointer_cast_chain()"
}

pub fn demo() -> (u8, usize) {
    (numeric_cast(), pointer_cast_chain())
}
