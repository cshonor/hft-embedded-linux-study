use std::rc::Rc;
use std::sync::Arc;
use std::thread;

/// 演示 Rc 的基本使用：共享所有权和引用计数
fn demo_rc_basic() {
    println!("=== Rc 基本使用示例 ===");
    
    // Rc::new 在堆上创建数据，返回栈上的智能指针
    // 计数器初始值为 1
    let rc1 = Rc::new(42);
    println!("创建 rc1，引用计数: {}", Rc::strong_count(&rc1)); // 应该是 1
    
    // Rc::clone 创建新的栈上指针，指向同一个堆数据
    // 计数器会加 1，但不会复制数据本身
    let rc2 = Rc::clone(&rc1);
    println!("克隆 rc2 后，引用计数: {}", Rc::strong_count(&rc1)); // 应该是 2
    
    let rc3 = Rc::clone(&rc1);
    println!("克隆 rc3 后，引用计数: {}", Rc::strong_count(&rc1)); // 应该是 3
    
    // 所有指针都指向同一个数据
    println!("rc1 的值: {}", rc1);
    println!("rc2 的值: {}", rc2);
    println!("rc3 的值: {}", rc3);
    
    // 当 rc2 和 rc3 离开作用域时，计数器会递减
    // 最后当 rc1 也离开作用域时，计数器归 0，数据被释放
    println!();
}

/// 演示 Rc 只能共享读，不能修改
fn demo_rc_readonly() {
    println!("=== Rc 只能共享读 ===");
    
    let rc = Rc::new(42);
    let rc_clone = Rc::clone(&rc);
    
    // 可以读取
    println!("读取 rc: {}", rc);
    println!("读取 rc_clone: {}", rc_clone);
    
    // 不能直接修改（下面的代码会编译错误）
    // *rc = 100;  // 错误！Rc 包裹的数据是不可变的
    
    println!("Rc 只能共享读，不能直接修改数据\n");
}

/// 演示 Rc 不能跨线程使用（会编译错误）
/// 注意：这个函数中的代码会编译失败，所以注释掉了
fn demo_rc_not_thread_safe() {
    println!("=== Rc 不能跨线程使用 ===");
    
    let _rc = Rc::new(42);
    
    // 尝试把 Rc 传到另一个线程会编译错误
    // 取消下面的注释会看到编译错误：
    // thread::spawn(move || {
    //     println!("在子线程中: {}", _rc);
    // });
    
    println!("Rc 不是线程安全的，不能跨线程传递\n");
}

/// 演示 Arc（线程安全的引用计数）
fn demo_arc_thread_safe() {
    println!("=== Arc 线程安全示例 ===");
    
    // Arc::new 创建线程安全的引用计数
    let arc = Arc::new(42);
    println!("创建 arc，引用计数: {}", Arc::strong_count(&arc));
    
    // Arc::clone 同样只增加计数，不复制数据
    let arc_clone1 = Arc::clone(&arc);
    let arc_clone2 = Arc::clone(&arc);
    println!("克隆后，引用计数: {}", Arc::strong_count(&arc));
    
    // Arc 可以安全地跨线程传递
    let handle1 = thread::spawn(move || {
        println!("线程1读取: {}", arc_clone1);
    });
    
    let handle2 = thread::spawn(move || {
        println!("线程2读取: {}", arc_clone2);
    });
    
    handle1.join().unwrap();
    handle2.join().unwrap();
    
    println!("Arc 可以安全地在多线程间共享\n");
}

/// 演示 Rc 和 Arc 的区别总结
fn demo_comparison() {
    println!("=== Rc vs Arc 总结 ===");
    println!("Rc (std::rc::Rc):");
    println!("  - 单线程使用");
    println!("  - 引用计数不是线程安全的");
    println!("  - 性能稍好（因为不需要原子操作）");
    println!("  - 不能跨线程传递");
    println!();
    println!("Arc (std::sync::Arc):");
    println!("  - 多线程使用");
    println!("  - 引用计数是线程安全的（使用原子操作）");
    println!("  - 性能稍差（因为需要原子操作）");
    println!("  - 可以跨线程传递");
    println!();
    println!("两者都只能共享不可变数据（只能读）");
    println!("如果需要修改，需要配合：");
    println!("  - Rc<RefCell<T>>  (单线程可变)");
    println!("  - Arc<Mutex<T>>   (多线程可变)");
    println!("  - Arc<RwLock<T>>  (多线程可变，支持多读)");
}

pub fn main() {
    demo_rc_basic();
    demo_rc_readonly();
    demo_rc_not_thread_safe();
    demo_arc_thread_safe();
    demo_comparison();
}

