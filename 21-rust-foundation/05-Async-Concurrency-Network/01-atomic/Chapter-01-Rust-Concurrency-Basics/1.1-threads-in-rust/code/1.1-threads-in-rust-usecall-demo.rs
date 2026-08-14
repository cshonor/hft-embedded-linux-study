use std::thread;

// 展示如何把普通函数指针传给 thread::spawn。
fn greet(name: &str) {
    println!("Hello from {name} thread!");
}

pub fn main() {
    // 直接把函数放到新线程里调用。
    let handle = thread::spawn(|| greet("child"));

    // 主线程继续执行自己的逻辑。
    greet("main");

    // 等待子线程结束。
    handle.join().unwrap();
}

