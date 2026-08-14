//! 6.2 测试引用计数：`clone` 后 drop 不提前释放。

use std::sync::atomic::{AtomicUsize, Ordering};

static DROPS: AtomicUsize = AtomicUsize::new(0);

struct CountOnDrop;

impl Drop for CountOnDrop {
    fn drop(&mut self) {
        DROPS.fetch_add(1, Ordering::Relaxed);
    }
}

pub fn demo() {
    DROPS.store(0, Ordering::Relaxed);
    {
        let _a = std::sync::Arc::new(CountOnDrop);
        let _b = _a.clone();
    }
    assert_eq!(DROPS.load(Ordering::Relaxed), 1);
    println!("=== 6.2 Arc drop once after two clones (drops={}) ===", 1);
}
