//! 可空指针优化：`Option<extern "C" fn>` 的 None = null。

pub type OptionalCallback = Option<extern "C" fn(i32) -> i32>;

extern "C" fn triple(x: i32) -> i32 {
    x * 3
}

pub fn invoke_optional(cb: OptionalCallback, x: i32) -> Option<i32> {
    cb.map(|f| f(x))
}

pub fn demo_niche() -> (Option<i32>, usize) {
    let none_size = std::mem::size_of::<OptionalCallback>();
    let with = invoke_optional(Some(triple), 4);
    let _without = invoke_optional(None, 4);
    (with, none_size)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn option_fn_niche() {
        assert_eq!(invoke_optional(Some(triple), 2), Some(6));
        assert_eq!(invoke_optional(None, 2), None);
        assert_eq!(std::mem::size_of::<OptionalCallback>(), std::mem::size_of::<usize>());
    }
}
