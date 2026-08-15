//! RAII Guard：panic 时恢复中间状态，保证 minimal exception safety。

/// 模拟「已取出旧值、尚未提交新值」的中间态；panic 时把旧值放回。
struct Hole<'a, T> {
    value: Option<T>,
    slot: &'a mut Option<T>,
}

impl<'a, T> Drop for Hole<'a, T> {
    fn drop(&mut self) {
        if let Some(v) = self.value.take() {
            *self.slot = Some(v);
        }
    }
}

pub fn swap_with_guard(slot: &mut Option<String>, new_val: String) -> Option<String> {
    let old = slot.replace(new_val);
    let mut hole = Hole {
        value: old,
        slot,
    };
    // 正常路径：消费 hole 中的 old，不再恢复
    hole.value.take()
}

/// 若此处 panic，Hole::drop 会把 old 写回 slot。
pub fn guarded_operation(slot: &mut Option<String>) -> String {
    let prev = swap_with_guard(slot, String::from("committed"));
    prev.unwrap_or_else(|| String::from("(none)"))
}
