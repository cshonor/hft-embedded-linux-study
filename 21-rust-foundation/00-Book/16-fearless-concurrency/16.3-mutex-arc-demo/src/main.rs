use std::sync::{Arc, Mutex};
use std::thread;

/// 示例 16-12：单线程中熟悉 `Mutex`；`MutexGuard` 离开作用域自动解锁。
fn example_mutex_single_thread() {
    println!("\n=== 16-12: Mutex（单线程）===");
    let m = Mutex::new(5);

    {
        let mut num = m.lock().unwrap();
        *num = 6;
    }

    println!("m = {m:?}");
}

/// 示例 16-15：`Arc<Mutex<i32>>`，10 个线程各 `+1`，结果为 10。
fn example_arc_mutex_counter() {
    println!("\n=== 16-15: Arc<Mutex>，10 线程 ===");
    let counter = Arc::new(Mutex::new(0));
    let mut handles = vec![];

    for _ in 0..10 {
        let counter = Arc::clone(&counter);
        let handle = thread::spawn(move || {
            let mut num = counter.lock().unwrap();
            *num += 1;
        });
        handles.push(handle);
    }

    for handle in handles {
        handle.join().unwrap();
    }

    println!("Result: {}", *counter.lock().unwrap());
}

fn main() {
    example_mutex_single_thread();
    example_arc_mutex_counter();
}
