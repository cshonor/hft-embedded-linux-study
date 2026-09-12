//! Layer 1 练手：纯计算函数，用于「同一份源码 → `.ll` / `.wasm` / `.wat`」对照。
//!
//! 选纯计算（无 I/O、无分配、无 panic 路径）是刻意的：
//! 这样 `.ll` 和 `.wat` 里剩下的几乎只有算术 + load/store，便于逐条比对。
//!
//! 两个函数形成对比：
//! - [`sma_update`]   纯算术（无内存访问）→ 看寄存器上的浮点运算
//! - [`weighted_mean`] 带循环 + 线性内存读取 → 看 i32 偏移寻址与循环结构

/// 滚动均价的 O(1) 增量更新：`新均值 = (旧和 − 移出价 + 新价) / 窗口长度`。
///
/// 纯算术，无分支无内存访问 —— 在 `.ll` 里应该只剩几行 `fadd`/`fsub`/`fdiv`。
#[no_mangle]
pub extern "C" fn sma_update(sum: f64, len: u32, out_price: f64, in_price: f64) -> f64 {
    if len == 0 {
        return 0.0;
    }
    (sum - out_price + in_price) / len as f64
}

/// 加权均值：越靠近窗口末端的报价权重越高（`(i + 1)` 线性权重）。
///
/// - `prices` 是线性内存里的 `f64` 数组首地址，`len` 为元素个数。
/// - **关键观察点**：在 Wasm 里指针是 **i32 偏移**（32 位地址空间），
///   与 native 目标下的 64 位指针完全不同 —— 这正是 Layer 1 要建立的直觉。
/// - 返回 `0.0` 表示空/无效输入（避免引入 panic 路径污染 IR）。
///
/// # Safety
/// `prices` 必须指向至少 `len` 个连续 `f64`，否则是 UB。
/// （Nomicon 的裸指针纪律；本函数刻意保留 unsafe 以对照 04-Rust-Nomicon。）
#[no_mangle]
pub unsafe extern "C" fn weighted_mean(prices: *const f64, len: usize) -> f64 {
    if prices.is_null() || len == 0 {
        return 0.0;
    }
    let slice = std::slice::from_raw_parts(prices, len);

    let mut acc = 0.0_f64;
    let mut wsum = 0.0_f64;
    for (i, p) in slice.iter().enumerate() {
        let w = (i + 1) as f64;
        acc += *p * w;
        wsum += w;
    }

    if wsum == 0.0 {
        0.0
    } else {
        acc / wsum
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sma_update_shifts_the_window_by_one() {
        // 窗口 [1,2,3] 和 = 6；移出 1 移入 4 → (6-1+4)/3 = 3
        assert!((sma_update(6.0, 3, 1.0, 4.0) - 3.0).abs() < 1e-12);
    }

    #[test]
    fn sma_update_rejects_empty_window() {
        assert_eq!(sma_update(6.0, 0, 1.0, 4.0), 0.0);
    }

    #[test]
    fn weighted_mean_weights_later_prices_higher() {
        let prices = [10.0_f64, 20.0, 30.0];
        let got = unsafe { weighted_mean(prices.as_ptr(), prices.len()) };
        // (10*1 + 20*2 + 30*3) / (1+2+3) = 140/6
        assert!((got - 140.0 / 6.0).abs() < 1e-12);
    }

    #[test]
    fn weighted_mean_handles_null_and_empty() {
        assert_eq!(unsafe { weighted_mean(std::ptr::null(), 4) }, 0.0);
        let prices = [1.0_f64];
        assert_eq!(unsafe { weighted_mean(prices.as_ptr(), 0) }, 0.0);
    }
}
