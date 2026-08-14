use std::thread;

/// 闭包捕获与 `move` 的简单线程示例。
pub fn run() {
    let x = 1;
    let y = 2;
    let z = 3;
    let f =thread::spawn(move || {
        println!("x: {}", x);
        println!("y: {}", y);
        println!("z: {}", z);
    });
    f.join().unwrap();
}
