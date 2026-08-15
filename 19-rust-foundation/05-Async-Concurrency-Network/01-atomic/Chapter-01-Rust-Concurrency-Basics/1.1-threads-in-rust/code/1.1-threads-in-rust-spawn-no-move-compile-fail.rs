//! 故意触发 `spawn` 无 `move` 的编译错误 — **本文件不应通过编译**。
//!
//! 运行对照：
//! ```text
//! rustc 1.1-threads-in-rust-spawn-no-move-compile-fail.rs
//! ```
//!
//! 期望报错（Rust 1.8x 实测）：
//! ```text
//! error[E0373]: closure may outlive the current function, but it borrows `local_var`,
//!               which is owned by the current function
//!   ...
//! note: function requires argument type to outlive `'static`
//! help: use the `move` keyword
//! ```
//!
//! 正确修复见 [1.1.3.5 §五](../1.1.3.5-move-capture-errors.md#五正确写法move-转移所有权)
//! 与 [move-closure-demo.rs](./1.1-threads-in-rust-move-closure-demo.rs)。

use std::thread;

fn main() {
    let local_var = String::from("我是 main 里的局部变量");

    // 不加 move：闭包默认捕获 &local_var → F: 'static 不满足 → 编译器在此拦住
    let handle = thread::spawn(|| {
        println!("子线程打印: {local_var}");
    });

    handle.join().unwrap();
}
