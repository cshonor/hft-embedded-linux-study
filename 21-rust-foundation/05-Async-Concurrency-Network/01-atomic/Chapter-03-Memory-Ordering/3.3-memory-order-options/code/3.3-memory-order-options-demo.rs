use std::sync::atomic::{AtomicBool, AtomicUsize, AtomicU32, Ordering};
use std::thread;
use std::time::Duration;

/// 演示 SeqCst 的全局顺序一致性
fn demo_seqcst_global_order() {
    println!("=== SeqCst 的全局顺序一致性 ===");
    
    static FLAG: AtomicBool = AtomicBool::new(false);
    static COUNTER: AtomicUsize = AtomicUsize::new(0);
    
    // 线程1：设置标志
    let t1 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(10));
        FLAG.store(true, Ordering::SeqCst); // 操作A
        println!("线程1: 执行操作A（设置FLAG为true）");
    });
    
    // 线程2：读取标志并计数
    let t2 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(20));
        let flag = FLAG.load(Ordering::SeqCst); // 操作B
        if flag {
            COUNTER.fetch_add(1, Ordering::SeqCst); // 操作C
            println!("线程2: 执行操作B（读FLAG）→ 操作C（计数+1）");
        }
    });
    
    // 线程3：读取计数
    let t3 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(30));
        let count = COUNTER.load(Ordering::SeqCst); // 操作D
        println!("线程3: 执行操作D（读计数），值: {}", count);
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    t3.join().unwrap();
    
    println!("说明：所有线程对 SeqCst 操作的顺序达成一致");
    println!("      操作A（设置FLAG）→ 操作B（读FLAG）→ 操作C（计数）→ 操作D（读计数）");
    println!();
}

/// 演示多个原子类使用 SeqCst 时的全局顺序
fn demo_multiple_atomics_seqcst() {
    println!("=== 多个原子类使用 SeqCst 的全局顺序 ===");
    
    static FLAG1: AtomicBool = AtomicBool::new(false);
    static FLAG2: AtomicBool = AtomicBool::new(false);
    static COUNTER: AtomicUsize = AtomicUsize::new(0);
    
    // 线程1：操作多个原子类
    let t1 = thread::spawn(|| {
        FLAG1.store(true, Ordering::SeqCst);
        println!("线程1: FLAG1.store(true, SeqCst)");
        thread::sleep(Duration::from_millis(10));
        COUNTER.fetch_add(1, Ordering::SeqCst);
        println!("线程1: COUNTER.fetch_add(1, SeqCst)");
    });
    
    // 线程2：操作多个原子类
    let t2 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(5));
        FLAG2.store(true, Ordering::SeqCst);
        println!("线程2: FLAG2.store(true, SeqCst)");
        thread::sleep(Duration::from_millis(10));
        COUNTER.fetch_add(1, Ordering::SeqCst);
        println!("线程2: COUNTER.fetch_add(1, SeqCst)");
    });
    
    // 线程3：读取所有原子类
    let t3 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(30));
        let f1 = FLAG1.load(Ordering::SeqCst);
        let f2 = FLAG2.load(Ordering::SeqCst);
        let cnt = COUNTER.load(Ordering::SeqCst);
        println!("线程3: FLAG1={}, FLAG2={}, COUNTER={}", f1, f2, cnt);
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    t3.join().unwrap();
    
    println!("说明：所有 SeqCst 操作（无论属于哪个原子类）形成统一的全局顺序");
    println!("      所有线程都认可同一套执行顺序");
    println!();
}

/// 演示 while 循环如何影响 SeqCst 的顺序
fn demo_while_loop_affects_seqcst() {
    println!("=== while 循环如何影响 SeqCst 的顺序 ===");
    
    static INIT_DONE: AtomicBool = AtomicBool::new(false);
    static WORK_COUNT: AtomicUsize = AtomicUsize::new(0);
    
    // 线程1：初始化完成后设置标志
    let t1 = thread::spawn(|| {
        println!("线程1: 开始初始化...");
        thread::sleep(Duration::from_millis(50)); // 模拟初始化耗时
        INIT_DONE.store(true, Ordering::SeqCst); // 操作A：设置初始化完成
        println!("线程1: 初始化完成，设置 INIT_DONE=true");
    });
    
    // 线程2：等待初始化完成后才开始工作
    let t2 = thread::spawn(|| {
        println!("线程2: 等待初始化完成...");
        
        // while 循环：主动等待操作A完成
        // 这个循环会影响线程2的执行时机，进而影响SeqCst的全局顺序
        while !INIT_DONE.load(Ordering::SeqCst) {
            // 循环等待，直到 INIT_DONE 变为 true
            thread::sleep(Duration::from_millis(1));
        }
        
        // 操作B：初始化完成后才开始工作
        println!("线程2: 检测到初始化完成，开始工作");
        WORK_COUNT.fetch_add(1, Ordering::SeqCst);
        println!("线程2: 工作完成，WORK_COUNT+1");
    });
    
    // 线程3：读取工作计数
    let t3 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(100));
        let count = WORK_COUNT.load(Ordering::SeqCst);
        println!("线程3: 读取 WORK_COUNT={}", count);
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    t3.join().unwrap();
    
    println!("说明：");
    println!("  1. while 循环是业务逻辑阻塞，改变线程2的执行时机");
    println!("  2. 循环让操作B只能在操作A之后执行");
    println!("  3. CPU根据实际执行时机形成 'A→B' 的顺序");
    println!("  4. SeqCst 把这个顺序同步给所有线程，保证一致性");
    println!();
}

/// 对比 SeqCst 和 Release/Acquire
fn demo_seqcst_vs_release_acquire() {
    println!("=== SeqCst vs Release/Acquire ===");
    
    println!("Release/Acquire（点对点同步）:");
    println!("  - 适合：少量线程间的定向同步");
    println!("  - 特点：只保证成对操作的可见性");
    println!("  - 例子：锁的加锁（Acquire）和解锁（Release）");
    println!("  - 性能：高");
    println!();
    
    println!("SeqCst（全局顺序一致性）:");
    println!("  - 适合：多个线程需要全局共识的场景");
    println!("  - 特点：所有 SeqCst 操作形成统一的全局顺序");
    println!("  - 例子：分布式共识、全局状态机、多操作依赖");
    println!("  - 性能：较低（需要同步更多操作的顺序）");
    println!();
    
    println!("选择建议：");
    println!("  - 大多数场景用 Release/Acquire 就够了");
    println!("  - 只有需要全局顺序统一时才用 SeqCst");
    println!();
}

/// 演示 SeqCst 的应用场景：多步骤依赖
fn demo_seqcst_multi_step_dependency() {
    println!("=== SeqCst 应用场景：多步骤依赖 ===");
    
    static INIT_FLAG: AtomicBool = AtomicBool::new(false);
    static WORK_COUNT: AtomicUsize = AtomicUsize::new(0);
    static TOTAL_COUNT: AtomicUsize = AtomicUsize::new(0);
    static DONE_FLAG: AtomicBool = AtomicBool::new(false);
    
    // 线程1：初始化 → 设置标志
    let t1 = thread::spawn(|| {
        println!("线程1: 执行操作A（初始化）");
        thread::sleep(Duration::from_millis(20));
        INIT_FLAG.store(true, Ordering::SeqCst); // 操作A
        println!("线程1: 操作A完成，设置 INIT_FLAG=true");
    });
    
    // 线程2：等待初始化 → 开始工作 → 累加计数
    let t2 = thread::spawn(|| {
        // 等待操作A完成
        while !INIT_FLAG.load(Ordering::SeqCst) {
            thread::sleep(Duration::from_millis(1));
        }
        
        println!("线程2: 执行操作B（读INIT_FLAG）");
        println!("线程2: 执行操作C（开始工作，WORK_COUNT+1）");
        WORK_COUNT.fetch_add(1, Ordering::SeqCst); // 操作C
        
        thread::sleep(Duration::from_millis(10));
        TOTAL_COUNT.fetch_add(1, Ordering::SeqCst); // 操作D
        println!("线程2: 执行操作D（累加总计数）");
    });
    
    // 线程3：等待总计数达到阈值 → 结束
    let t3 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(50));
        let total = TOTAL_COUNT.load(Ordering::SeqCst);
        println!("线程3: 执行操作E（读TOTAL_COUNT），值: {}", total);
        
        if total >= 1 {
            DONE_FLAG.store(true, Ordering::SeqCst); // 操作F
            println!("线程3: 执行操作F（设置DONE_FLAG=true）");
        }
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    t3.join().unwrap();
    
    println!("说明：SeqCst 保证多步骤依赖的顺序");
    println!("      操作A（初始化）→ 操作B（读标志）→ 操作C（工作）→ 操作D（累加）→ 操作E（读计数）→ 操作F（结束）");
    println!("      所有线程都认可这个顺序，不会出现中间步骤乱序");
    println!();
}

/// 演示 SeqCst 不主动等待，但保证顺序一致性
fn demo_seqcst_no_active_wait() {
    println!("=== SeqCst 不主动等待，但保证顺序一致性 ===");
    
    static DATA: AtomicUsize = AtomicUsize::new(0);
    
    // 线程1：写数据
    let t1 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(10));
        DATA.store(100, Ordering::SeqCst);
        println!("线程1: 写入 DATA=100");
    });
    
    // 线程2：读数据（不等待，直接读）
    let t2 = thread::spawn(|| {
        // 不等待，直接读
        let value = DATA.load(Ordering::SeqCst);
        println!("线程2: 读取 DATA={}（可能读到旧值0）", value);
        
        // 如果需要新值，需要主动等待
        println!("线程2: 如果需要新值，需要主动循环等待...");
        let mut attempts = 0;
        while value == 0 && attempts < 10 {
            thread::sleep(Duration::from_millis(5));
            let new_value = DATA.load(Ordering::SeqCst);
            if new_value != 0 {
                println!("线程2: 循环等待后读到新值 DATA={}", new_value);
                return;
            }
            attempts += 1;
        }
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    
    println!("说明：");
    println!("  - SeqCst 不主动让线程等待");
    println!("  - 但保证所有线程对操作顺序达成一致");
    println!("  - 如果需要等待，需要自己写循环逻辑");
    println!();
}

/// 演示 SeqCst 的"中央仓库"特性
fn demo_seqcst_central_warehouse() {
    println!("=== SeqCst 的'中央仓库'特性 ===");
    
    static WAREHOUSE: AtomicUsize = AtomicUsize::new(0);
    
    // 多个线程同时操作"中央仓库"
    let mut handles = vec![];
    
    for i in 0..5 {
        let handle = thread::spawn(move || {
            thread::sleep(Duration::from_millis(i * 10));
            let old = WAREHOUSE.fetch_add(1, Ordering::SeqCst);
            println!("线程{}: 从中央仓库读取 {}, 写入 {}", i, old, old + 1);
        });
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    let final_value = WAREHOUSE.load(Ordering::SeqCst);
    println!("最终值: {}", final_value);
    
    println!("说明：");
    println!("  - 所有 SeqCst 操作都会同步到'中央仓库'");
    println!("  - 所有线程看到的操作顺序完全一致");
    println!("  - 就像所有人都去同一个仓库查看，看到的记录都一样");
    println!();
}

/// 总结 SeqCst 的核心要点
fn demo_seqcst_summary() {
    println!("=== SeqCst 核心要点总结 ===");
    println!();
    println!("1. 全局顺序一致性：");
    println!("   - 所有 SeqCst 操作形成统一的全局顺序");
    println!("   - 所有线程都认可同一套执行顺序");
    println!("   - 不限于单个原子类，跨原子类也统一");
    println!();
    println!("2. 顺序的形成：");
    println!("   - 运行时动态生成（根据线程调度、CPU负载）");
    println!("   - 通过内存屏障和缓存同步实现");
    println!("   - while 循环等业务逻辑会影响执行时机，间接影响顺序");
    println!();
    println!("3. 不主动等待：");
    println!("   - SeqCst 不阻塞线程");
    println!("   - 只保证顺序和结果的可见性");
    println!("   - 需要等待时，自己写循环逻辑");
    println!();
    println!("4. 应用场景：");
    println!("   - 需要全局统一顺序的场景");
    println!("   - 多步骤依赖的同步");
    println!("   - 分布式共识、全局状态机");
    println!();
    println!("5. 性能考虑：");
    println!("   - SeqCst 性能比 Release/Acquire 低");
    println!("   - 大多数场景用 Release/Acquire 就够了");
    println!("   - 只有真正需要全局顺序时才用 SeqCst");
    println!();
}

pub fn main() {
    demo_seqcst_global_order();
    demo_multiple_atomics_seqcst();
    demo_while_loop_affects_seqcst();
    demo_seqcst_vs_release_acquire();
    demo_seqcst_multi_step_dependency();
    demo_seqcst_no_active_wait();
    demo_seqcst_central_warehouse();
    demo_seqcst_summary();
}

