use std::thread;

// 简单的递归函数：计算第 n 个斐波那契数。
fn fibonacci(n: u32) -> u32 {
    match n {
        0 => 0,
        1 => 1,
        _ => fibonacci(n - 1) + fibonacci(n - 2),
    }
}

pub fn main() {
    // 在线程里执行递归函数，返回结果给主线程。
    let handle = thread::spawn(|| fibonacci(10));

    // join 会返回线程闭包的返回值。
    let result = handle.join().unwrap();
    println!("fib(10) = {result}");
}

