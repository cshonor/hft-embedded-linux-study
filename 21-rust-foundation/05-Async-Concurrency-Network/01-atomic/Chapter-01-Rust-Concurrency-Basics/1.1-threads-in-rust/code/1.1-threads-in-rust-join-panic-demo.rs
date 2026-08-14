//! §1.1.2 · `join()` 捕获子线程 panic · `downcast_ref`
use std::any::Any;
use std::thread::{self, JoinHandle};

pub fn main() {
    demo_join_panic_static_str();
    demo_join_panic_format_string();
    demo_join_panic_unknown_type();
    demo_join_and_print_helper();
}

/// `panic!("字面量")` → 载荷为 `&'static str`
pub fn demo_join_panic_static_str() {
    let h = thread::spawn(|| {
        panic!("线程崩溃信息");
    });

    match h.join() {
        Ok(v) => println!("线程正常返回: {v:?}"),
        Err(e) => print_panic_msg(&e),
    }
}

/// `panic!("…{}", x)` 格式化 → 载荷常为 `String`
pub fn demo_join_panic_format_string() {
    let h = thread::spawn(|| panic!("数值{}", 666));
    join_and_print(h);
}

/// `panic_any(999)` → 载荷为 `i32`，`&str` / `String` 均匹配不到
pub fn demo_join_panic_unknown_type() {
    let h = thread::spawn(|| std::panic::panic_any(999i32));
    join_and_print(h);
}

pub fn demo_join_and_print_helper() {
    let h1 = thread::spawn(|| panic!("出错了"));
    join_and_print(h1);

    let h2 = thread::spawn(|| panic!("格式化: {}", "via String"));
    join_and_print(h2);
}

pub fn join_and_print<T: std::fmt::Debug>(handle: JoinHandle<T>) {
    match handle.join() {
        Ok(val) => println!("线程返回: {val:?}"),
        Err(panic_err) => print_panic_msg(&panic_err),
    }
}

pub fn print_panic_msg(err: &Box<dyn Any + Send>) {
    if let Some(s) = err.downcast_ref::<&str>() {
        // s: &&str — 见 1.1.2 §2.2
        eprintln!("panic(&str): {s}");
    } else if let Some(s) = err.downcast_ref::<String>() {
        eprintln!("panic(String): {s}");
    } else {
        eprintln!("panic 未知类型，type_id: {:?}", err.type_id());
    }
}

/// 若希望 panic 继续向上传播（主线程也崩溃）：
#[allow(dead_code)]
fn resume_unwind_example<T>(handle: JoinHandle<T>) {
    if let Err(e) = handle.join() {
        std::panic::resume_unwind(e);
    }
}
