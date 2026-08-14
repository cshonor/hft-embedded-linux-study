//! Drop flags：条件初始化时，仅在实际构造时于作用域末 Drop。

struct LoudDrop(&'static str);

impl Drop for LoudDrop {
    fn drop(&mut self) {
        println!("  [drop] {}", self.0);
    }
}

/// `flag == false` 时不构造 → 运行时不调用 Drop（drop flag = 未初始化）。
pub fn conditional_drop(flag: bool) {
    let x;
    if flag {
        x = LoudDrop("conditional");
    }
}

/// 重新赋值：先 Drop 旧值再写入新值。
pub fn reassignment_drops_old() {
    let mut x = LoudDrop("first");
    x = LoudDrop("second");
}
