//! 递归 Drop、`ManuallyDrop`、`Option::take` 打破默认清理。

use std::mem::ManuallyDrop;

struct Inner;
struct Outer(Inner);

impl Drop for Inner {
    fn drop(&mut self) {
        println!("    drop Inner (field)");
    }
}

impl Drop for Outer {
    fn drop(&mut self) {
        println!("    drop Outer (body first, then fields)");
    }
}

pub fn recursive_drop_order() {
    let _o = Outer(Inner);
}

pub fn manually_drop_skip() {
    struct Loud;
    impl Drop for Loud {
        fn drop(&mut self) {
            println!("    Loud::drop (should NOT run)");
        }
    }
    let _slot = ManuallyDrop::new(Loud);
    // `_slot` 离开作用域时不调用 `Loud::drop`。
    println!("    leaving scope without Loud::drop");
}

/// `Option::take` 移出字段，避免 Drop 时再次 drop 已移走值。
pub fn option_take_breaks_field_drop() -> Option<String> {
    let mut slot = Some(String::from("owned"));
    slot.take()
}
