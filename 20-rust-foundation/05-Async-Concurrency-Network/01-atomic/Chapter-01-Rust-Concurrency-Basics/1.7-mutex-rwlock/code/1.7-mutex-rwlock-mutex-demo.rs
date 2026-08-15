use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

/// 演示 Mutex::new 返回的对象类型
fn demo_mutex_new() {
    println!("=== Mutex::new 返回的对象 ===");
    
    // Mutex::new(0) 返回 Mutex<i32> 类型
    // 这是一个"带锁的盒子"，里面装着 i32 类型的数据
    let _mutex: Mutex<i32> = Mutex::new(0);
    println!("Mutex::new(0) 返回的类型: Mutex<i32>");
    println!("这是一个互斥锁包装器，内部持有数据并管理锁状态");
    
    println!();
}

/// 演示 lock() 方法返回的 MutexGuard
fn demo_mutex_guard() {
    println!("=== lock() 返回的 MutexGuard ===");
    
    let mutex = Mutex::new(5);
    
    // lock() 返回 Result<MutexGuard<i32>, PoisonError>
    // unwrap() 后得到 MutexGuard<i32>
    let mut guard = mutex.lock().unwrap();
    
    println!("lock().unwrap() 返回的类型: MutexGuard<i32>");
    println!("MutexGuard 的作用:");
    println!("  1. 标记当前线程持有了锁");
    println!("  2. 实现了 DerefMut，可以解引用修改内部数据");
    println!("  3. 离开作用域时自动释放锁（Drop trait）");
    
    // 通过解引用修改数据（因为实现了 DerefMut）
    *guard += 10;
    println!("通过 *guard += 10 修改后，值: {}", *guard);
    
    // guard 离开作用域，自动释放锁
    drop(guard);
    println!("guard 被销毁，锁自动释放");
    
    println!();
}

/// 演示多线程累加示例（完整版）
fn demo_multithread_increment() {
    println!("=== 多线程累加示例 ===");
    
    // Arc<Mutex<i32>>: Arc 负责跨线程共享，Mutex 负责线程安全
    let n = Arc::new(Mutex::new(0));
    let mut handles = vec![];
    
    // 创建 10 个线程
    for _ in 0..10 {
        // 下划线 _ 表示忽略循环变量（只需要循环 10 次，不需要用到 0,1,2...9）
        let n_clone = Arc::clone(&n);
        
        let handle = thread::spawn(move || {
            // 每个线程内部循环 100 次
            for _ in 0..100 {
                // lock() 返回 Result<MutexGuard<i32>, PoisonError>
                // unwrap() 得到 MutexGuard<i32>
                let mut guard = n_clone.lock().unwrap();
                
                // *guard 解引用，因为 MutexGuard 实现了 DerefMut
                *guard += 1;
                
                // guard 离开作用域，自动释放锁
            }
        });
        
        handles.push(handle);
    }
    
    // 等待所有线程完成
    for handle in handles {
        handle.join().unwrap();
    }
    
    // 最终结果应该是 10 * 100 = 1000
    let final_value = n.lock().unwrap();
    println!("10 个线程各执行 100 次 +1，最终结果: {}", *final_value);
    
    println!();
}

/// 演示 into_inner() 方法：消耗 Mutex，取出内部数据
fn demo_into_inner() {
    println!("=== into_inner() 方法：消耗 Mutex ===");
    
    let mutex = Mutex::new(42);
    
    println!("创建 Mutex<i32>，内部值: 42");
    
    // into_inner() 消耗（consume）Mutex，取出内部数据
    // 注意：参数是 self，不是 &self，所以会转移所有权
    let data = mutex.into_inner().unwrap();
    
    println!("into_inner() 后，取出数据: {}", data);
    println!("Mutex 已被消耗，无法再使用");
    
    // 下面这行会编译错误！因为 mutex 的所有权已被转移
    // let guard = mutex.lock().unwrap(); // 错误！
    
    println!("消耗（consume）的含义:");
    println!("  - 不是作用域结束，而是主动转移所有权");
    println!("  - Mutex 被拆解，取出内部数据");
    println!("  - 原 Mutex 变量永久失效，无法再使用");
    
    println!();
}

/// 演示 DerefMut 特质的作用
fn demo_derefmut() {
    println!("=== DerefMut 特质：智能解引用 ===");
    
    let mutex = Mutex::new(100);
    
    let mut guard = mutex.lock().unwrap();
    println!("guard 的类型: MutexGuard<i32>");
    
    // 因为 MutexGuard<i32> 实现了 DerefMut
    // 所以可以直接用 *guard 来修改内部数据
    *guard += 50;
    println!("通过 *guard += 50 修改后，值: {}", *guard);
    
    println!();
    println!("DerefMut 的作用:");
    println!("  - 不是指针或引用，而是一个转换协议");
    println!("  - 让 MutexGuard 可以像 &mut T 一样使用");
    println!("  - 编译器自动调用 deref_mut() 方法");
    println!("  - *guard += 1 等价于 guard.deref_mut() += 1");
    
    println!();
}

/// 演示线程休眠：thread::sleep
fn demo_thread_sleep() {
    println!("=== 线程休眠：thread::sleep ===");
    
    println!("开始时间");
    
    // 让当前线程休眠 1 秒
    // 注意：是 std::thread::sleep，不是 scoped freeze
    thread::sleep(Duration::from_secs(1));
    
    println!("1 秒后");
    
    // 也可以休眠毫秒
    thread::sleep(Duration::from_millis(500));
    
    println!("再 500 毫秒后");
    
    println!();
    println!("thread::sleep 的特点:");
    println!("  - 阻塞当前线程，期间不执行任何操作");
    println!("  - 需要传入 Duration 类型参数");
    println!("  - 常用方法: from_secs(), from_millis(), from_micros()");
    
    println!();
}

/// 演示带休眠的多线程示例
fn demo_sleep_in_threads() {
    println!("=== 带休眠的多线程示例 ===");
    
    let shared_data = Arc::new(Mutex::new(0));
    let mut handles = vec![];
    
    for i in 0..3 {
        let data_clone = Arc::clone(&shared_data);
        
        let handle = thread::spawn(move || {
            // 每个线程休眠不同时间后修改数据
            thread::sleep(Duration::from_millis(100 * (i + 1) as u64));
            
            let mut guard = data_clone.lock().unwrap();
            *guard += 1;
            println!("线程 {} 修改后，值: {}", i, *guard);
        });
        
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("所有线程完成");
    
    println!();
}

/// 总结所有概念
fn demo_summary() {
    println!("=== 核心概念总结 ===");
    println!();
    println!("1. Mutex::new(T) -> Mutex<T>");
    println!("   - 返回一个互斥锁包装器");
    println!("   - 内部持有数据并管理锁状态");
    println!();
    println!("2. mutex.lock() -> Result<MutexGuard<T>, PoisonError>");
    println!("   - 获取锁，返回 MutexGuard");
    println!("   - MutexGuard 实现了 DerefMut，可以解引用修改数据");
    println!("   - MutexGuard 离开作用域时自动释放锁");
    println!();
    println!("3. mutex.into_inner() -> Result<T, PoisonError>");
    println!("   - 消耗 Mutex，取出内部数据");
    println!("   - 原 Mutex 永久失效");
    println!();
    println!("4. DerefMut 特质");
    println!("   - 让 MutexGuard 可以像 &mut T 一样使用");
    println!("   - 编译器自动处理解引用");
    println!();
    println!("5. thread::sleep(Duration)");
    println!("   - 让当前线程休眠指定时间");
    println!("   - 阻塞线程，不执行任何操作");
    println!();
}

pub fn main() {
    demo_mutex_new();
    demo_mutex_guard();
    demo_multithread_increment();
    demo_into_inner();
    demo_derefmut();
    demo_thread_sleep();
    demo_sleep_in_threads();
    demo_summary();
}

