//! `mem::forget`：不调用 Drop。内存安全，但资源泄漏。

use std::mem;

pub fn forget_box() {
    let b = Box::new(vec![1, 2, 3]);
    mem::forget(b);
    // 内存泄漏，但无 UAF
}

pub fn forget_is_safe_but_leaks() -> &'static str {
    "forget skips Drop; safe Rust may still leak (deadlock, Rc cycles, forget)"
}
