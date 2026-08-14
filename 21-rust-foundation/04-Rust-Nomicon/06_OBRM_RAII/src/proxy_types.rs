//! 代理类型：`Drain` 被 forget 时 std 的 leak amplification。

use std::mem;

/// 中途 forget `Drain`：被 drain 的元素泄漏，但 `Vec` 仍保持可用（leak amplification）。
pub fn forget_drain() -> Vec<i32> {
    let mut v = vec![10, 20, 30];
    let drain = v.drain(0..2);
    mem::forget(drain);
    v.push(40);
    v
}

// Rc：恶意 forget 大量 Clone 可能导致 refcount 溢出 → UAF（勿在生产复现）。
// thread::scoped（已移除）：forget 守卫 → 父栈销毁、子线程 UAF → API 废弃。

pub fn proxy_warning() -> &'static str {
    "see 00-overview.md §3: Drain / Rc / scoped"
}
