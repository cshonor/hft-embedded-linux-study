use std::sync::{Arc, Mutex};

/// 仅用于在编译期检查 `T` 是否满足 `Send`。
fn assert_send<T: Send>() {}

/// 仅用于在编译期检查 `T` 是否满足 `Sync`。
fn assert_sync<T: Sync>() {}

fn main() {
    assert_send::<i32>();
    assert_sync::<i32>();

    assert_send::<Arc<Mutex<i32>>>();
    assert_sync::<Arc<Mutex<i32>>>();

    // `std::rc::Rc<T>` 非 `Send` / `Sync`，若写 `assert_send::<Rc<i32>>()` 则不能编译。

    println!("Send/Sync 边界检查通过（见源码中的 assert_* 与注释）。");
}
