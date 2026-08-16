//! §1.1.3 · `spawn` 必须 `F: 'static` vs `thread::scope` 可借栈
use std::thread;

pub fn main() {
    demo_spawn_move_fix();
    demo_safe_scope_borrow();
}

/// `thread::spawn`：不能借局部引用 → 须 **`move`** 拿所有权。
pub fn demo_spawn_move_fix() {
    let local = String::from("hello");
    let h = thread::spawn(move || {
        println!("spawn (move): {local}");
    });
    h.join().unwrap();
}

/// `thread::scope`：阻塞到子线程全结束，函数内局部变量可借（`F: 'scope`）。
pub fn demo_safe_scope_borrow() {
    let s = String::from("局部变量");
    thread::scope(|scope| {
        scope
            .spawn(|| println!("scope (borrow): {s}"))
            .join()
            .unwrap();
    });
    // scope 返回后 s 才随栈帧销毁
}

// 取消注释 → 编译错误：`create_thread` 返回后 s 已 drop，F 不满足 'static
//
// fn create_thread() {
//     let s = String::from("仅存活于本函数");
//     let _h = thread::spawn(|| println!("{s}"));
// }
//
// pub fn demo_create_thread_would_fail() {
//     create_thread();
//     thread::sleep(std::time::Duration::from_millis(100));
// }
