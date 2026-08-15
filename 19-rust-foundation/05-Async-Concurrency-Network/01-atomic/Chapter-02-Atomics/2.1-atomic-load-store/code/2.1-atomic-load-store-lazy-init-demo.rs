use std::sync::atomic::{AtomicU64, Ordering};
use std::thread;
use std::time::Duration;

/// 模拟耗时的初始化计算
fn calculate_x() -> u64 {
    println!("  正在执行 calculate_x()... 这是一个耗时操作");
    thread::sleep(Duration::from_millis(100)); // 模拟耗时
    println!("  calculate_x() 完成，返回 42");
    42
}

/// 有问题的延迟初始化实现（多线程下可能重复初始化）
fn get_x_unsafe() -> u64 {
    static X: AtomicU64 = AtomicU64::new(0); // 0 表示未初始化
    
    // 1. 加载当前值（Relaxed：只需要原子读，不需要顺序约束）
    let mut x = X.load(Ordering::Relaxed);
    
    // 2. 如果 X 还是 0（未初始化），就计算并存储
    if x == 0 {
        x = calculate_x(); // 耗时计算
        // 3. 存储结果（Relaxed：只保证原子写）
        X.store(x, Ordering::Relaxed);
    }
    
    x
}

/// 安全的延迟初始化实现（使用 compare_exchange 避免重复初始化）
fn get_x_safe() -> u64 {
    static X: AtomicU64 = AtomicU64::new(0); // 0 表示未初始化
    
    // 1. 先加载当前值
    let mut x = X.load(Ordering::Relaxed);
    
    if x == 0 {
        // 2. 计算结果（不管多少线程进来，都先算）
        let computed = calculate_x();
        
        // 3. 关键：用 compare_exchange 原子地判断并设置
        // 如果 X 当前是 0，就把它设为 computed；如果不是 0，说明其他线程已经初始化了
        match X.compare_exchange(
            0,                      // 期望 X 当前是 0
            computed,               // 如果是 0，就把 X 设为 computed
            Ordering::Relaxed,     // 成功时的内存序
            Ordering::Relaxed      // 失败时的内存序
        ) {
            Ok(_) => {
                // 成功：当前线程是第一个初始化的，使用自己计算的值
                x = computed;
            }
            Err(already_init) => {
                // 失败：其他线程已经初始化了，使用已存在的值
                println!("  检测到其他线程已初始化，使用已有值");
                x = already_init;
            }
        }
    }
    
    x
}

/// 演示不安全的延迟初始化（多线程下会重复计算）
fn demo_unsafe_lazy_init() {
    println!("=== 不安全的延迟初始化（可能重复计算）===");
    
    let mut handles = vec![];
    
    // 创建 5 个线程，同时调用 get_x_unsafe
    for i in 0..5 {
        let handle = thread::spawn(move || {
            println!("线程 {}: 调用 get_x_unsafe()", i);
            let result = get_x_unsafe();
            println!("线程 {}: 得到结果 {}", i, result);
        });
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("问题：多个线程可能同时检测到 x==0，导致重复计算");
    println!();
}

/// 演示安全的延迟初始化（使用 compare_exchange）
fn demo_safe_lazy_init() {
    println!("=== 安全的延迟初始化（使用 compare_exchange）===");
    
    let mut handles = vec![];
    
    // 创建 5 个线程，同时调用 get_x_safe
    for i in 0..5 {
        let handle = thread::spawn(move || {
            println!("线程 {}: 调用 get_x_safe()", i);
            let result = get_x_safe();
            println!("线程 {}: 得到结果 {}", i, result);
        });
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("优点：即使多个线程同时进入，也只会计算一次");
    println!();
}

/// 演示 compare_exchange 的工作原理
fn demo_compare_exchange() {
    println!("=== compare_exchange 工作原理 ===");
    
    static COUNTER: AtomicU64 = AtomicU64::new(0);
    
    // compare_exchange 的签名：
    // compare_exchange(expected, new, success_ordering, failure_ordering)
    // - 如果当前值 == expected，就设为 new，返回 Ok(旧值)
    // - 如果当前值 != expected，返回 Err(当前值)
    
    // 场景1：当前值是 0，期望是 0，成功
    let result1 = COUNTER.compare_exchange(0, 10, Ordering::Relaxed, Ordering::Relaxed);
    println!("场景1: compare_exchange(0, 10) = {:?}", result1);
    println!("  COUNTER 当前值: {}", COUNTER.load(Ordering::Relaxed));
    
    // 场景2：当前值是 10，期望是 0，失败
    let result2 = COUNTER.compare_exchange(0, 20, Ordering::Relaxed, Ordering::Relaxed);
    println!("场景2: compare_exchange(0, 20) = {:?}", result2);
    println!("  COUNTER 当前值: {}", COUNTER.load(Ordering::Relaxed));
    
    // 场景3：当前值是 10，期望是 10，成功
    let result3 = COUNTER.compare_exchange(10, 30, Ordering::Relaxed, Ordering::Relaxed);
    println!("场景3: compare_exchange(10, 30) = {:?}", result3);
    println!("  COUNTER 当前值: {}", COUNTER.load(Ordering::Relaxed));
    
    println!();
    println!("说明：compare_exchange 是原子操作，保证只有一个线程能成功");
    println!();
}

/// 演示 Relaxed 内存序在延迟初始化中的使用
fn demo_relaxed_ordering() {
    println!("=== Relaxed 内存序在延迟初始化中的使用 ===");
    
    println!("为什么延迟初始化可以用 Relaxed？");
    println!("  1. 只需要保证原子操作本身（load/store 不会读一半写一半）");
    println!("  2. 不需要约束指令顺序（初始化顺序不影响结果）");
    println!("  3. 只需要最终一致（最终所有线程都能读到初始化后的值）");
    println!();
    println!("Relaxed 的特点：");
    println!("  - 只保证原子性，不约束顺序");
    println!("  - 性能最好，CPU 可以自由优化");
    println!("  - 适合：计数器、标志位、延迟初始化");
    println!();
    println!("如果要用 Release/Acquire：");
    println!("  - Release：保证初始化操作在 store 之前完成");
    println!("  - Acquire：保证读取操作在 load 之后执行");
    println!("  - 但在这个场景下，Relaxed 已经足够，不需要额外约束");
    println!();
}

/// 演示 thread::current() 和 park/unpark
fn demo_thread_current_park() {
    println!("=== thread::current() 和 park/unpark ===");
    
    // 主线程获取自己的句柄
    let main_thread = thread::current();
    println!("主线程 ID: {:?}", main_thread.id());
    
    // 创建子线程，传入主线程句柄
    let handle = thread::spawn(move || {
        println!("子线程: 启动，2秒后唤醒主线程...");
        thread::sleep(Duration::from_secs(2));
        
        // 用主线程句柄唤醒主线程
        main_thread.unpark();
        println!("子线程: 已发送唤醒信号");
    });
    
    println!("主线程: 进入循环，调用 park() 阻塞等待...");
    
    // 主线程 park 自己，直到被唤醒
    thread::park();
    println!("主线程: 被唤醒啦！");
    
    handle.join().unwrap();
    println!("说明：thread::current() 返回调用它的线程句柄");
    println!("      park() 让当前线程休眠，unpark() 唤醒指定线程");
    println!();
}

/// 演示 thread::sleep 的作用范围
fn demo_thread_sleep() {
    println!("=== thread::sleep 的作用范围 ===");
    
    println!("主线程: 开始");
    
    let handle = thread::spawn(|| {
        println!("子线程: 开始，准备休眠 1 秒");
        thread::sleep(Duration::from_secs(1));
        println!("子线程: 休眠结束");
    });
    
    println!("主线程: 继续执行（不等待子线程）");
    thread::sleep(Duration::from_millis(500));
    println!("主线程: 主线程休眠 0.5 秒");
    
    handle.join().unwrap();
    println!("说明：thread::sleep 只影响调用它的线程");
    println!("      主线程和子线程各自休眠各自的，互不影响");
    println!();
}

/// 总结延迟初始化的关键点
fn demo_summary() {
    println!("=== 延迟初始化总结 ===");
    println!();
    println!("核心概念：");
    println!("  - 延迟初始化：变量在第一次使用时才创建");
    println!("  - 用原子变量标记状态（0 = 未初始化，非 0 = 已初始化）");
    println!("  - 多线程下需要避免重复初始化");
    println!();
    println!("关键方法：");
    println!("  - load(Relaxed)：加载当前值");
    println!("  - store(Relaxed)：存储值");
    println!("  - compare_exchange：原子地比较并交换，避免竞态条件");
    println!();
    println!("内存序选择：");
    println!("  - Relaxed：适合延迟初始化（只需要原子性，不需要顺序约束）");
    println!("  - Release/Acquire：适合需要保证顺序的场景");
    println!();
    println!("常见应用：");
    println!("  - 单例模式");
    println!("  - 配置加载");
    println!("  - 数据库连接池初始化");
    println!("  - 缓存初始化");
    println!();
}

pub fn main() {
    demo_unsafe_lazy_init();
    thread::sleep(Duration::from_millis(500)); // 间隔一下
    demo_safe_lazy_init();
    demo_compare_exchange();
    demo_relaxed_ordering();
    demo_thread_current_park();
    demo_thread_sleep();
    demo_summary();
}

