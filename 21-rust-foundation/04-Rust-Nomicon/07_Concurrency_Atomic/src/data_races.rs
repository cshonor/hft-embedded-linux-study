//! 数据竞争 vs 竞争条件；TOCTOU + `get_unchecked` 风险（注释）。

use std::sync::{Arc, Mutex};

/// Safe：检查与访问在同一借用/锁保护下，无数据竞争。
pub fn safe_get(v: &Arc<Mutex<Vec<i32>>>, i: usize) -> Option<i32> {
    let guard = v.lock().unwrap();
    guard.get(i).copied()
}

// TOCTOU + unsafe 反模式（勿写进生产）：
//
//   if i < len { unsafe { buf.get_unchecked(i) } }
//   ^ 检查与 unsafe 访问之间若另一线程可改 len → 越界 UB
//
// 对策：持锁 / 原子长度 / 单次切片 `get` 在同一临界区完成。

pub fn race_condition_is_safe_but_wrong() -> &'static str {
    "deadlock and logic races are safe (no UB) but still bugs"
}

pub fn demo_safe_shared() -> Option<i32> {
    let v = Arc::new(Mutex::new(vec![10, 20, 30]));
    safe_get(&v, 1)
}
