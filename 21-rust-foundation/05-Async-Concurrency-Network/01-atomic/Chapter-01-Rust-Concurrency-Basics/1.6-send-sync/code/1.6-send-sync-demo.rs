use std::thread;
use std::sync::{Arc, Mutex};
use std::rc::Rc;
use std::cell::RefCell;

/// 演示 Send trait：所有权可以跨线程转移
fn demo_send_trait() {
    println!("=== Send trait 示例：所有权跨线程转移 ===");
    
    // 1. 实现了 Send 的类型（如 i32, String）
    let num = 42;
    let text = String::from("hello");
    
    // 可以把所有权转移到子线程
    let handle = thread::spawn(move || {
        println!("子线程拿到 num 本身: {}", num);
        println!("子线程拿到 text 本身: {}", text);
        // 子线程拥有这些变量的所有权，可以直接使用
    });
    handle.join().unwrap();
    
    // 注意：move 后，主线程不能再使用 text（所有权已转移）
    // println!("{}", text); // 编译错误！所有权已转移
    
    println!();
}

/// 演示未实现 Send 的类型（如 Rc）
fn demo_not_send() {
    println!("=== 未实现 Send 的类型（Rc）===");
    
    let _rc = Rc::new(42);
    
    // 尝试把 Rc 转移到子线程会编译错误
    // 取消下面的注释会看到编译错误：
    // thread::spawn(move || {
    //     println!("{}", _rc); // 错误：`Rc<i32>` cannot be sent between threads safely
    // });
    
    println!("Rc 没有实现 Send，不能跨线程转移所有权");
    println!("需要使用 Arc（原子引用计数）代替");
    println!();
}

/// 演示 Sync trait：引用可以跨线程共享
fn demo_sync_trait() {
    println!("=== Sync trait 示例：引用跨线程共享 ===");
    
    // 使用 Arc 来共享数据（Arc 实现了 Sync）
    // 这样多个线程可以同时持有同一个数据的引用
    let shared_num = Arc::new(100);
    
    // 多个线程可以同时共享同一个 Arc 的引用
    let num_clone1 = Arc::clone(&shared_num);
    let handle1 = thread::spawn(move || {
        println!("线程1读取: {}", num_clone1);
    });
    
    let num_clone2 = Arc::clone(&shared_num);
    let handle2 = thread::spawn(move || {
        println!("线程2读取: {}", num_clone2);
    });
    
    handle1.join().unwrap();
    handle2.join().unwrap();
    
    // 主线程还能继续使用（只是共享引用，所有权没转移）
    println!("主线程读取: {}", shared_num);
    
    println!();
}

/// 演示未实现 Sync 的类型（如 RefCell）
fn demo_not_sync() {
    println!("=== 未实现 Sync 的类型（RefCell）===");
    
    let _ref_cell = RefCell::new(42);
    
    // 尝试把 RefCell 的引用共享给多个线程会编译错误
    // 取消下面的注释会看到编译错误：
    // let ref1 = &_ref_cell;
    // thread::spawn(move || {
    //     ref1.borrow(); // 错误：`RefCell<i32>` cannot be shared between threads safely
    // });
    
    println!("RefCell 没有实现 Sync，不能跨线程共享引用");
    println!("需要使用 Mutex 或 RwLock 代替");
    println!();
}

/// 演示 Send 和 Sync 的组合使用：Arc + Mutex
fn demo_send_and_sync_combined() {
    println!("=== Send 和 Sync 组合使用：Arc + Mutex ===");
    
    // Arc<Mutex<T>> 同时实现了 Send 和 Sync
    // - Arc 实现了 Send + Sync（可以跨线程传递和共享）
    // - Mutex<T> 实现了 Sync（可以跨线程共享引用）
    let shared_data = Arc::new(Mutex::new(0));
    let mut handles = vec![];
    
    // 创建多个线程，每个线程都要修改共享数据
    for i in 0..5 {
        // Arc::clone 只是增加引用计数，不复制数据
        // 多个线程共享同一个 Mutex 的引用
        let data_clone = Arc::clone(&shared_data);
        
        let handle = thread::spawn(move || {
            // 通过 lock() 获取锁，才能修改数据
            let mut num = data_clone.lock().unwrap();
            *num += 1;
            println!("线程 {} 修改后: {}", i, *num);
        });
        
        handles.push(handle);
    }
    
    // 等待所有线程完成
    for handle in handles {
        handle.join().unwrap();
    }
    
    // 主线程还能继续使用（所有权没转移，只是共享了引用）
    let final_value = shared_data.lock().unwrap();
    println!("最终值: {}", *final_value);
    
    println!();
}

/// 演示 Send 和 Sync 的区别总结
fn demo_send_vs_sync() {
    println!("=== Send vs Sync 核心区别 ===");
    println!();
    println!("Send（所有权转移）:");
    println!("  - 子线程拿到的是变量本身的所有权");
    println!("  - 主线程之后不能再使用这个变量");
    println!("  - 例子：String, i32, Vec<T> 等");
    println!("  - 用法：thread::spawn(move || {{ 使用变量 }})");
    println!();
    println!("Sync（引用共享）:");
    println!("  - 子线程拿到的是变量的不可变引用（&T）");
    println!("  - 主线程还能继续使用原变量");
    println!("  - 例子：&i32, &str, Arc<T>, Mutex<T> 等");
    println!("  - 用法：多个线程共享同一个引用");
    println!();
    println!("关键关系:");
    println!("  - 如果 T 实现了 Sync，那么 &T 一定实现了 Send");
    println!("  - 如果 T 实现了 Send，T 本身可以在线程间转移");
    println!("  - Arc<T> 实现了 Send + Sync（可以跨线程传递和共享）");
    println!("  - Mutex<T> 实现了 Sync（可以跨线程共享引用）");
    println!("  - Rc<T> 没有实现 Send（单线程引用计数）");
    println!("  - RefCell<T> 没有实现 Sync（单线程内部可变性）");
    println!();
}

/// 演示实际应用场景
fn demo_practical_example() {
    println!("=== 实际应用场景 ===");
    
    // 场景：多个线程需要共享一个计数器
    let counter = Arc::new(Mutex::new(0));
    let mut handles = vec![];
    
    // 创建 10 个线程，每个线程增加计数器
    for _ in 0..10 {
        let counter_clone = Arc::clone(&counter);
        
        let handle = thread::spawn(move || {
            // 获取锁，修改计数器
            let mut count = counter_clone.lock().unwrap();
            *count += 1;
            // 锁在这里自动释放（离开作用域）
        });
        
        handles.push(handle);
    }
    
    // 等待所有线程完成
    for handle in handles {
        handle.join().unwrap();
    }
    
    // 读取最终结果
    let final_count = counter.lock().unwrap();
    println!("10 个线程各增加 1 次，最终计数: {}", *final_count);
    
    println!();
    println!("为什么用 Arc<Mutex<T>>？");
    println!("  - Arc: 让多个线程能共享同一个 Mutex 的引用（Sync）");
    println!("  - Mutex: 保证同一时间只有一个线程能修改数据（线程安全）");
    println!("  - 组合起来：既能跨线程共享，又能安全修改");
}

pub fn main() {
    demo_send_trait();
    demo_not_send();
    demo_sync_trait();
    demo_not_sync();
    demo_send_and_sync_combined();
    demo_send_vs_sync();
    demo_practical_example();
}

