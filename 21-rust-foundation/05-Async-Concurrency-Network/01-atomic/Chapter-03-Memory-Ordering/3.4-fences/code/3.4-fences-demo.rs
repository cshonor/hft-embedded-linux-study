use std::sync::atomic::{fence, AtomicBool, Ordering};
use std::thread;
use std::time::Duration;

/// 演示 SeqCst 栅栏在单线程内的作用
fn demo_fence_single_thread() {
    println!("=== SeqCst 栅栏在单线程内的作用 ===");
    
    static mut DATA: u32 = 0;
    static FLAG: AtomicBool = AtomicBool::new(false);
    
    // 单线程内：写数据 → 栅栏 → 写标志位
    unsafe {
        DATA = 42; // 操作A：栅栏上方
        fence(Ordering::SeqCst); // 栅栏：保证A在B之前
        FLAG.store(true, Ordering::Relaxed); // 操作B：栅栏下方
    }
    
    println!("说明：栅栏保证单线程内，上方操作（写DATA）在下方操作（写FLAG）之前");
    println!("      即使编译器/CPU想重排，栅栏也会阻止");
    println!();
}

/// 演示 SeqCst 栅栏在多线程间的作用
fn demo_fence_multithread() {
    println!("=== SeqCst 栅栏在多线程间的作用 ===");
    
    static mut DATA: u32 = 0;
    static FLAG: AtomicBool = AtomicBool::new(false);
    
    // 线程1：写数据 → 栅栏 → 写标志位
    let t1 = thread::spawn(|| {
        unsafe {
            DATA = 100; // 操作A：栅栏上方
            println!("线程1: 执行操作A（写DATA=100）");
        }
        fence(Ordering::SeqCst); // 栅栏：保证A在B之前，且同步给所有线程
        FLAG.store(true, Ordering::Relaxed); // 操作B：栅栏下方
        println!("线程1: 执行操作B（写FLAG=true）");
    });
    
    // 线程2：读标志位 → 栅栏 → 读数据
    let t2 = thread::spawn(|| {
        // 等待标志位
        while !FLAG.load(Ordering::Relaxed) {
            thread::sleep(Duration::from_millis(1));
        }
        println!("线程2: 执行操作C（读FLAG=true）");
        
        fence(Ordering::SeqCst); // 栅栏：保证C在D之前，且同步给所有线程
        unsafe {
            let data = DATA; // 操作D：栅栏下方
            println!("线程2: 执行操作D（读DATA={}）", data);
        }
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    
    println!("说明：");
    println!("  - 栅栏不能调节线程之间的执行顺序（线程1和线程2可能交替执行）");
    println!("  - 但栅栏能保证操作级的顺序：A→B→C→D");
    println!("  - 所有线程都认可这个操作顺序，不会出现乱序");
    println!();
}

/// 演示栅栏的关键操作是"上下两部分"的整体顺序
fn demo_fence_upper_lower() {
    println!("=== 栅栏的关键操作：上下两部分的整体顺序 ===");
    
    static mut DATA1: u32 = 0;
    static mut DATA2: u32 = 0;
    static FLAG: AtomicBool = AtomicBool::new(false);
    
    // 线程1：栅栏上方有多个操作 → 栅栏 → 栅栏下方有多个操作
    let t1 = thread::spawn(|| {
        unsafe {
            DATA1 = 10; // 栅栏上方操作1
            DATA2 = 20; // 栅栏上方操作2
            println!("线程1: 栅栏上方操作完成（DATA1=10, DATA2=20）");
        }
        
        fence(Ordering::SeqCst); // 栅栏：分隔符
        
        FLAG.store(true, Ordering::Relaxed); // 栅栏下方操作1
        println!("线程1: 栅栏下方操作完成（FLAG=true）");
    });
    
    // 线程2：读标志位 → 栅栏 → 读数据
    let t2 = thread::spawn(|| {
        while !FLAG.load(Ordering::Relaxed) {
            thread::sleep(Duration::from_millis(1));
        }
        println!("线程2: 栅栏上方操作完成（读FLAG=true）");
        
        fence(Ordering::SeqCst); // 栅栏：分隔符
        
        unsafe {
            let d1 = DATA1; // 栅栏下方操作1
            let d2 = DATA2; // 栅栏下方操作2
            println!("线程2: 栅栏下方操作完成（读DATA1={}, DATA2={}）", d1, d2);
        }
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    
    println!("说明：");
    println!("  - 栅栏的关键操作不是单个操作，而是'上方所有操作'和'下方所有操作'的整体");
    println!("  - 中央仓库保证：线程1的'上方操作'→'下方操作'，线程2的'上方操作'→'下方操作'");
    println!("  - 跨线程顺序：线程1的'下方操作'→线程2的'上方操作'");
    println!();
}

/// 演示栅栏不能调节线程执行顺序，但能保证操作顺序
fn demo_fence_thread_vs_operation_order() {
    println!("=== 栅栏：线程顺序 vs 操作顺序 ===");
    
    static mut DATA: u32 = 0;
    static FLAG: AtomicBool = AtomicBool::new(false);
    
    println!("场景：线程1和线程2可能交替执行，但操作顺序是固定的");
    
    // 线程1：写数据 → 栅栏 → 写标志位
    let t1 = thread::spawn(|| {
        unsafe {
            DATA = 200;
            println!("线程1: 操作A（写DATA=200）");
        }
        thread::sleep(Duration::from_millis(10)); // 模拟耗时
        fence(Ordering::SeqCst);
        FLAG.store(true, Ordering::Relaxed);
        println!("线程1: 操作B（写FLAG=true）");
    });
    
    // 线程2：读标志位 → 栅栏 → 读数据
    let t2 = thread::spawn(|| {
        thread::sleep(Duration::from_millis(5)); // 可能先被调度
        println!("线程2: 尝试操作C（读FLAG），但可能还在等待...");
        
        while !FLAG.load(Ordering::Relaxed) {
            thread::sleep(Duration::from_millis(1));
        }
        println!("线程2: 操作C完成（读FLAG=true）");
        
        fence(Ordering::SeqCst);
        unsafe {
            let data = DATA;
            println!("线程2: 操作D（读DATA={}）", data);
        }
    });
    
    t1.join().unwrap();
    t2.join().unwrap();
    
    println!("说明：");
    println!("  - 栅栏不能调节线程执行顺序（线程2可能先被调度）");
    println!("  - 但栅栏能保证操作顺序：A→B→C→D（不管线程怎么交替执行）");
    println!("  - 这是'操作级顺序'，不是'线程级顺序'");
    println!();
}

/// 对比 SeqCst 栅栏和 Release/Acquire 栅栏
fn demo_fence_seqcst_vs_release_acquire() {
    println!("=== SeqCst 栅栏 vs Release/Acquire 栅栏 ===");
    
    println!("SeqCst 栅栏：");
    println!("  - 范围：所有使用 SeqCst 的线程");
    println!("  - 保证：全局操作顺序统一");
    println!("  - 性能：较低（需要同步更多操作）");
    println!("  - 适用：需要全局共识的场景");
    println!();
    
    println!("Release/Acquire 栅栏：");
    println!("  - 范围：成对使用的两个线程");
    println!("  - 保证：写后读的可见性");
    println!("  - 性能：较高（只同步两个线程）");
    println!("  - 适用：单向同步场景");
    println!();
    
    println!("选择建议：");
    println!("  - 大多数场景用 Release/Acquire 就够了");
    println!("  - 只有需要全局顺序统一时才用 SeqCst");
    println!();
}

/// 演示栅栏的顺序参数
fn demo_fence_ordering_parameters() {
    println!("=== 栅栏的顺序参数 ===");
    
    println!("SeqCst：");
    println!("  - 最严格，保证全局操作顺序统一");
    println!("  - 所有线程都认可同一套顺序");
    println!("  - 适合：需要全局共识的场景");
    println!();
    
    println!("Release：");
    println!("  - 用在写操作之后");
    println!("  - 保证：栅栏前的所有写操作，对后续 Acquire 栅栏可见");
    println!("  - 必须和 Acquire 配对使用");
    println!();
    
    println!("Acquire：");
    println!("  - 用在读操作之后");
    println!("  - 保证：栅栏后的所有读操作，能看到 Release 栅栏前的写操作");
    println!("  - 必须和 Release 配对使用");
    println!();
    
    println!("Relaxed：");
    println!("  - 对栅栏无效！栅栏必须指定顺序");
    println!("  - 如果传 Relaxed，Rust 会报错");
    println!();
}

/// 演示栅栏的"中央仓库"特性
fn demo_fence_central_warehouse() {
    println!("=== 栅栏的'中央仓库'特性 ===");
    
    static mut DATA: u32 = 0;
    static FLAG: AtomicBool = AtomicBool::new(false);
    
    // 多个线程使用栅栏，所有操作都会记录到"中央仓库"
    let mut handles = vec![];
    
    // 线程1：写数据 → 栅栏 → 写标志位
    handles.push(thread::spawn(|| {
        unsafe { DATA = 300; }
        fence(Ordering::SeqCst);
        FLAG.store(true, Ordering::Relaxed);
        println!("线程1: 操作记录到中央仓库");
    }));
    
    // 线程2：读标志位 → 栅栏 → 读数据
    handles.push(thread::spawn(|| {
        while !FLAG.load(Ordering::Relaxed) {
            thread::sleep(Duration::from_millis(1));
        }
        fence(Ordering::SeqCst);
        unsafe {
            let data = DATA;
            println!("线程2: 从中央仓库读取，DATA={}", data);
        }
    }));
    
    // 线程3：也使用栅栏
    handles.push(thread::spawn(|| {
        thread::sleep(Duration::from_millis(20));
        fence(Ordering::SeqCst);
        println!("线程3: 操作也记录到中央仓库");
    }));
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("说明：");
    println!("  - 所有 SeqCst 栅栏的操作都会记录到'中央仓库'");
    println!("  - 所有线程查看仓库时，看到的顺序完全一致");
    println!("  - 就像所有人都去同一个仓库查看，看到的记录都一样");
    println!();
}

/// 总结栅栏的核心要点
fn demo_fence_summary() {
    println!("=== 栅栏核心要点总结 ===");
    println!();
    println!("1. 栅栏是静态函数，不需要实例：");
    println!("   - 直接调用 fence(Ordering::SeqCst)");
    println!("   - 作用范围是调用线程的所有内存操作");
    println!();
    println!("2. 栅栏的关键操作：");
    println!("   - 不是单个操作，而是'上方所有操作'和'下方所有操作'的整体");
    println!("   - 栅栏保证：上方操作 → 下方操作（不会被重排）");
    println!();
    println!("3. 栅栏不能调节线程执行顺序：");
    println!("   - 线程还是会被 CPU 交替调度");
    println!("   - 栅栏管不了'哪个线程先整体执行'");
    println!();
    println!("4. 栅栏能保证操作顺序：");
    println!("   - 单线程内：上方操作在下方操作之前");
    println!("   - 多线程间：所有线程认可同一套操作顺序");
    println!("   - 这是'操作级顺序'，不是'线程级顺序'");
    println!();
    println!("5. SeqCst 栅栏的范围：");
    println!("   - 覆盖所有使用 SeqCst 的线程");
    println!("   - 形成全局统一的'中央仓库'");
    println!("   - 所有线程查看时，看到的顺序完全一致");
    println!();
    println!("6. 顺序参数的作用：");
    println!("   - SeqCst：全局顺序统一");
    println!("   - Release/Acquire：成对使用，单向同步");
    println!("   - Relaxed：对栅栏无效");
    println!();
}

pub fn main() {
    demo_fence_single_thread();
    demo_fence_multithread();
    demo_fence_upper_lower();
    demo_fence_thread_vs_operation_order();
    demo_fence_seqcst_vs_release_acquire();
    demo_fence_ordering_parameters();
    demo_fence_central_warehouse();
    demo_fence_summary();
}

