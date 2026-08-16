use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::thread;
use std::time::Duration;

/// 演示 AtomicBool 作为停止标志
pub fn demo_atomic_bool_stop_flag() {
    println!("=== AtomicBool 作为停止标志 ===");
    
    // static 变量：全局唯一，生命周期和程序一样长
    // 大写命名：Rust 的常量和静态变量命名规范
    static STOP: AtomicBool = AtomicBool::new(false);
    
    // 创建子线程，持续执行任务
    let handle = thread::spawn(|| {
        while !STOP.load(Ordering::Relaxed) {
            // 只要 STOP 是 false，就继续执行任务
            println!("子线程正在工作...");
            thread::sleep(Duration::from_millis(500));
        }
        println!("子线程收到停止信号，退出循环");
    });
    
    // 主线程等待一段时间后，设置停止标志
    thread::sleep(Duration::from_secs(2));
    println!("主线程设置停止标志");
    STOP.store(true, Ordering::Relaxed);
    
    handle.join().unwrap();
    println!("说明：用 AtomicBool 作为多线程间的停止标志");
    println!();
}

/// 演示 Relaxed 顺序：不约束操作顺序
pub fn demo_ordering_relaxed() {
    println!("=== Ordering::Relaxed：不约束顺序 ===");
    
    static COUNTER: AtomicI32 = AtomicI32::new(0);
    
    // 多个线程同时增加计数
    let mut handles = vec![];
    for _ in 0..5 {
        let handle = thread::spawn(|| {
            for _ in 0..100 {
                // Relaxed：只保证原子操作本身，不约束顺序
                COUNTER.fetch_add(1, Ordering::Relaxed);
            }
        });
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("最终计数: {} (应该是 500)", COUNTER.load(Ordering::Relaxed));
    println!("说明：Relaxed 只保证原子性，不约束与其他操作的顺序");
    println!();
}

/// 演示 Release/Acquire 配对：保证顺序
pub fn demo_release_acquire() {
    println!("=== Release/Acquire 配对：保证顺序 ===");
    
    // 共享数据：初始是 0
    static DATA: AtomicI32 = AtomicI32::new(0);
    // 准备标志：初始是 false（没准备好）
    static READY: AtomicBool = AtomicBool::new(false);
    
    // 线程 A：准备数据
    let thread_a = thread::spawn(|| {
        println!("线程 A：开始准备数据");
        
        // 1. 准备数据：把 DATA 改成 100
        // 用 Relaxed 就行（因为只关心自己改对，不用约束顺序）
        DATA.store(100, Ordering::Relaxed);
        thread::sleep(Duration::from_millis(100)); // 模拟耗时操作
        
        // 2. 设 READY 为 true，必须用 Release！
        // 告诉 CPU/编译器："这行之前的所有操作（比如上面的 DATA.store），
        // 不准挪到这行后面"
        READY.store(true, Ordering::Release);
        println!("线程 A：数据准备完成，设置 READY=true");
    });
    
    // 线程 B：等待数据准备好，然后使用数据
    let thread_b = thread::spawn(|| {
        println!("线程 B：等待数据准备...");
        
        // 1. 循环读 READY，必须用 Acquire！
        // 告诉 CPU/编译器："只要读到 READY=true，
        // 这行后面的所有操作（比如下面的 DATA.load），不准挪到这行前面"
        while !READY.load(Ordering::Acquire) {
            thread::sleep(Duration::from_millis(10));
        }
        
        // 2. 读到 READY=true 后，读 DATA 的值
        let data = DATA.load(Ordering::Relaxed);
        println!("线程 B：读到数据 {}", data);
        
        // 保证：此时 data 一定是 100，不会是 0！
        assert_eq!(data, 100, "数据必须是 100！");
    });
    
    thread_a.join().unwrap();
    thread_b.join().unwrap();
    
    println!("说明：Release/Acquire 配对保证顺序不会被打乱");
    println!("  - Release：写操作之前的代码不能乱序到后面");
    println!("  - Acquire：读操作之后的代码不能乱序到前面");
    println!("  - 两者配合：建立同步点，保证数据传递顺序");
    println!();
}

/// 演示如果不用 Release/Acquire 可能出现的问题（理论上）
pub fn demo_why_need_release_acquire() {
    println!("=== 为什么需要 Release/Acquire ===");
    
    static DATA: AtomicI32 = AtomicI32::new(0);
    static READY: AtomicBool = AtomicBool::new(false);
    
    // 线程 A：准备数据
    let thread_a = thread::spawn(|| {
        // 准备数据
        DATA.store(100, Ordering::Relaxed);
        
        // 如果用 Relaxed 而不是 Release，理论上可能被乱序：
        // CPU/编译器可能把 READY.store(true) 放到 DATA.store(100) 前面
        // 这样线程 B 可能提前读到 READY=true，但 DATA 还是 0
        READY.store(true, Ordering::Relaxed); // 不推荐：可能乱序
    });
    
    // 线程 B：等待并使用数据
    let thread_b = thread::spawn(|| {
        while !READY.load(Ordering::Relaxed) { // 不推荐：可能乱序
            // 如果用 Relaxed 而不是 Acquire，理论上可能被乱序：
            // CPU/编译器可能把 DATA.load() 放到 READY.load() 前面
        }
        
        let data = DATA.load(Ordering::Relaxed);
        println!("线程 B 读到的数据: {} (理论上可能是 0 或 100)", data);
        
        // 注意：实际运行中可能不会出错，但理论上存在风险
        // 生产环境应该用 Release/Acquire 保证安全
    });
    
    thread_a.join().unwrap();
    thread_b.join().unwrap();
    
    println!("说明：用 Relaxed 理论上可能乱序，生产环境应使用 Release/Acquire");
    println!();
}

/// 演示从标准输入读取命令控制线程停止
pub fn demo_stdin_control() {
    println!("=== 从标准输入控制线程停止 ===");
    
    static STOP: AtomicBool = AtomicBool::new(false);
    
    // 创建子线程
    let handle = thread::spawn(|| {
        while !STOP.load(Ordering::Relaxed) {
            println!("子线程正在工作...");
            thread::sleep(Duration::from_millis(500));
        }
        println!("子线程收到停止信号");
    });
    
    // 主线程：从标准输入读取命令
    println!("输入 'stop' 可停止子线程");
    println!("（注意：这个示例会立即停止，实际使用时需要异步读取输入）");
    
    // 模拟输入 "stop"
    thread::sleep(Duration::from_millis(1000));
    STOP.store(true, Ordering::Relaxed);
    println!("主线程设置 STOP=true");
    
    handle.join().unwrap();
    
    println!("说明：AtomicBool 适合做简单的多线程控制标志");
    println!();
}

/// 总结 Ordering 的使用场景
pub fn demo_ordering_summary() {
    println!("=== Ordering 使用场景总结 ===");
    println!();
    println!("Relaxed（放松的）:");
    println!("  - 只保证原子操作本身的完整性");
    println!("  - 不约束与其他操作的顺序");
    println!("  - 适合：计数器、独立标志位");
    println!("  - 例子：统计访问次数、简单的停止标志");
    println!();
    println!("Release（释放）:");
    println!("  - 写操作之前的代码不能乱序到后面");
    println!("  - 必须和 Acquire 配对使用");
    println!("  - 适合：发布数据前的最后一步");
    println!();
    println!("Acquire（获取）:");
    println!("  - 读操作之后的代码不能乱序到前面");
    println!("  - 必须和 Release 配对使用");
    println!("  - 适合：获取数据后的第一步");
    println!();
    println!("Release + Acquire 配对:");
    println!("  - 建立同步点，保证数据传递顺序");
    println!("  - 适合：生产者-消费者、初始化-使用");
    println!("  - 例子：准备数据→设置标志→读取标志→使用数据");
    println!();
    println!("SeqCst（顺序一致性）:");
    println!("  - 最严格，所有线程看到一致的操作顺序");
    println!("  - 性能开销最大");
    println!("  - 适合：需要全局一致顺序的特殊场景");
    println!();
}

