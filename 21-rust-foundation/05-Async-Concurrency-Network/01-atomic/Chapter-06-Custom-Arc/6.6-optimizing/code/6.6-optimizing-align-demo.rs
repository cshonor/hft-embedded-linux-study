//! 6.6 布局优化：`#[repr(align(64))]` 将计数与热数据分到不同 cache line（示意）。

#[repr(align(64))]
struct PaddedCounter {
    count: std::sync::atomic::AtomicUsize,
}

struct HotData {
    value: u64,
}

pub fn demo() {
    let pad = PaddedCounter {
        count: std::sync::atomic::AtomicUsize::new(0),
    };
    let hot = HotData { value: 99 };
    pad.count.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
    println!(
        "=== 6.6 layout: counter size={}, HotData value={} ===",
        std::mem::size_of::<PaddedCounter>(),
        hot.value
    );
}
