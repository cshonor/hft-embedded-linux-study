use std::sync::atomic::{AtomicI32, Ordering};
use std::thread;
use std::time::Duration;

/// 演示 fetch_add：原子加操作，返回旧值
fn demo_fetch_add() {
    println!("=== fetch_add：原子加操作，返回旧值 ===");
    
    static COUNTER: AtomicI32 = AtomicI32::new(0);
    
    // fetch_add 返回操作前的旧值
    let old_value1 = COUNTER.fetch_add(5, Ordering::Relaxed);
    println!("fetch_add(5) 返回旧值: {}, 当前值: {}", old_value1, COUNTER.load(Ordering::Relaxed));
    
    let old_value2 = COUNTER.fetch_add(3, Ordering::Relaxed);
    println!("fetch_add(3) 返回旧值: {}, 当前值: {}", old_value2, COUNTER.load(Ordering::Relaxed));
    
    println!("说明：fetch_add 返回操作前的值，然后原子地加上指定值");
    println!();
}

/// 演示 fetch_sub：原子减操作，返回旧值
fn demo_fetch_sub() {
    println!("=== fetch_sub：原子减操作，返回旧值 ===");
    
    static COUNTER: AtomicI32 = AtomicI32::new(10);
    
    let old_value1 = COUNTER.fetch_sub(3, Ordering::Relaxed);
    println!("fetch_sub(3) 返回旧值: {}, 当前值: {}", old_value1, COUNTER.load(Ordering::Relaxed));
    
    let old_value2 = COUNTER.fetch_sub(2, Ordering::Relaxed);
    println!("fetch_sub(2) 返回旧值: {}, 当前值: {}", old_value2, COUNTER.load(Ordering::Relaxed));
    
    println!("说明：fetch_sub 返回操作前的值，然后原子地减去指定值");
    println!();
}

/// 演示为什么需要返回旧值：基于旧值做判断
fn demo_why_return_old_value() {
    println!("=== 为什么需要返回旧值：基于旧值做判断 ===");
    
    static COUNTER: AtomicI32 = AtomicI32::new(0);
    
    // 场景：只有当值小于 10 时才加 1
    loop {
        let old_value = COUNTER.fetch_add(1, Ordering::Relaxed);
        
        if old_value < 10 {
            println!("旧值 {} < 10，加 1 后变成 {}", old_value, old_value + 1);
        } else {
            // 如果已经 >= 10，需要回退（减回去）
            COUNTER.fetch_sub(1, Ordering::Relaxed);
            println!("旧值 {} >= 10，回退，最终值: {}", old_value, COUNTER.load(Ordering::Relaxed));
            break;
        }
        
        if old_value >= 9 {
            break;
        }
    }
    
    println!("说明：返回旧值让我们可以基于旧值做判断和决策");
    println!();
}

/// 演示多线程下的 fetch_add：返回旧值的实际应用
fn demo_fetch_add_multithread() {
    println!("=== 多线程下的 fetch_add：返回旧值的实际应用 ===");
    
    static COUNTER: AtomicI32 = AtomicI32::new(0);
    let mut handles = vec![];
    
    // 创建 3 个线程，每个线程加 10 次
    for i in 0..3 {
        let handle = thread::spawn(move || {
            for j in 0..10 {
                // fetch_add 返回操作前的旧值
                let old_value = COUNTER.fetch_add(1, Ordering::Relaxed);
                println!("线程 {}: 第 {} 次操作，旧值: {}, 新值: {}", 
                         i, j + 1, old_value, old_value + 1);
                thread::sleep(Duration::from_millis(10));
            }
        });
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("最终计数: {}", COUNTER.load(Ordering::Relaxed));
    println!("说明：每个线程都能拿到操作前的旧值，用于日志或判断");
    println!();
}

/// 演示 fetch_or、fetch_and、fetch_xor：按位操作
fn demo_bitwise_operations() {
    println!("=== 按位操作：fetch_or、fetch_and、fetch_xor ===");
    
    static FLAGS: AtomicI32 = AtomicI32::new(0b0000); // 二进制：0000
    
    // fetch_or：按位或，设置标志位
    let old1 = FLAGS.fetch_or(0b0001, Ordering::Relaxed); // 设置第 0 位
    println!("fetch_or(0b0001) 旧值: {:04b}, 新值: {:04b}", old1, FLAGS.load(Ordering::Relaxed));
    
    let old2 = FLAGS.fetch_or(0b0010, Ordering::Relaxed); // 设置第 1 位
    println!("fetch_or(0b0010) 旧值: {:04b}, 新值: {:04b}", old2, FLAGS.load(Ordering::Relaxed));
    
    // fetch_and：按位与，清除标志位
    let old3 = FLAGS.fetch_and(0b1110, Ordering::Relaxed); // 清除第 0 位
    println!("fetch_and(0b1110) 旧值: {:04b}, 新值: {:04b}", old3, FLAGS.load(Ordering::Relaxed));
    
    // fetch_xor：按位异或，翻转标志位
    let old4 = FLAGS.fetch_xor(0b0010, Ordering::Relaxed); // 翻转第 1 位
    println!("fetch_xor(0b0010) 旧值: {:04b}, 新值: {:04b}", old4, FLAGS.load(Ordering::Relaxed));
    
    println!("说明：按位操作也返回旧值，可以用于检查标志位状态");
    println!();
}

/// 演示 fetch_max 和 fetch_min：比较并更新
fn demo_fetch_max_min() {
    println!("=== fetch_max 和 fetch_min：比较并更新 ===");
    
    static MAX_VALUE: AtomicI32 = AtomicI32::new(0);
    static MIN_VALUE: AtomicI32 = AtomicI32::new(100);
    
    let values = [5, 10, 3, 15, 8];
    
    for &val in &values {
        // fetch_max：如果新值更大，就更新；否则不变
        let old_max = MAX_VALUE.fetch_max(val, Ordering::Relaxed);
        println!("fetch_max({}) 旧值: {}, 新值: {}", val, old_max, MAX_VALUE.load(Ordering::Relaxed));
        
        // fetch_min：如果新值更小，就更新；否则不变
        let old_min = MIN_VALUE.fetch_min(val, Ordering::Relaxed);
        println!("fetch_min({}) 旧值: {}, 新值: {}", val, old_min, MIN_VALUE.load(Ordering::Relaxed));
    }
    
    println!("最终最大值: {}", MAX_VALUE.load(Ordering::Relaxed));
    println!("最终最小值: {}", MIN_VALUE.load(Ordering::Relaxed));
    println!("说明：fetch_max/min 返回旧值，可以判断是否真的更新了");
    println!();
}

/// 演示 swap：原子交换，返回旧值
fn demo_swap() {
    println!("=== swap：原子交换，返回旧值 ===");
    
    static VALUE: AtomicI32 = AtomicI32::new(10);
    
    let old1 = VALUE.swap(20, Ordering::Relaxed);
    println!("swap(20) 返回旧值: {}, 当前值: {}", old1, VALUE.load(Ordering::Relaxed));
    
    let old2 = VALUE.swap(30, Ordering::Relaxed);
    println!("swap(30) 返回旧值: {}, 当前值: {}", old2, VALUE.load(Ordering::Relaxed));
    
    println!("说明：swap 用新值替换旧值，返回被替换的旧值");
    println!();
}

/// 演示返回旧值的实际应用场景
fn demo_practical_use_cases() {
    println!("=== 返回旧值的实际应用场景 ===");
    
    println!("1. 实现无锁计数器：");
    println!("   let old = counter.fetch_add(1, Relaxed);");
    println!("   if old == 0 {{ println!(\"计数器从 0 开始\"); }}");
    println!();
    
    println!("2. 实现自旋锁：");
    println!("   let old = lock.swap(1, Acquire);");
    println!("   if old == 0 {{ /* 获取锁成功 */ }}");
    println!();
    
    println!("3. 实现无锁栈：");
    println!("   let old_head = head.load(Relaxed);");
    println!("   loop {{");
    println!("       new_node.next = old_head;");
    println!("       if head.compare_exchange(old_head, new_node, ...) {{ break; }}");
    println!("   }}");
    println!();
    
    println!("4. 实现 ID 分配器：");
    println!("   let id = next_id.fetch_add(1, Relaxed);");
    println!("   // 返回的 id 是唯一的，即使多线程同时调用");
    println!();
}

/// 演示硬件原语的映射
fn demo_hardware_mapping() {
    println!("=== 硬件原语的映射 ===");
    
    println!("Rust 的原子操作直接映射到 CPU 硬件指令：");
    println!("  - fetch_add → x86 的 LOCK XADD 指令");
    println!("  - fetch_sub → x86 的 LOCK XADD（负数）");
    println!("  - swap → x86 的 XCHG 指令");
    println!("  - compare_exchange → x86 的 CMPXCHG 指令");
    println!();
    println!("这些硬件指令天然返回旧值：");
    println!("  - CPU 在执行原子操作时，会把旧值放到寄存器");
    println!("  - Rust 只是把这个值返回给开发者");
    println!("  - 没有额外的性能开销");
    println!();
}

/// 总结返回旧值的原因
fn demo_summary() {
    println!("=== 为什么 fetch_add/sub 返回旧值？===");
    println!();
    println!("1. 硬件原语的直接映射：");
    println!("   - CPU 指令（如 xadd）天然返回旧值");
    println!("   - Rust 直接暴露这个特性，无额外开销");
    println!();
    println!("2. 并发编程的实用需求：");
    println!("   - 状态依赖更新：基于旧值做判断");
    println!("   - 实现高级同步原语：无锁数据结构");
    println!("   - 避免二次竞争：不需要再次读取");
    println!();
    println!("3. 灵活性与性能的平衡：");
    println!("   - 需要旧值时：直接用返回值");
    println!("   - 不需要旧值时：忽略返回值即可");
    println!("   - 比先读再写更高效（单次原子操作）");
    println!();
}

pub fn main() {
    demo_fetch_add();
    demo_fetch_sub();
    demo_why_return_old_value();
    demo_fetch_add_multithread();
    demo_bitwise_operations();
    demo_fetch_max_min();
    demo_swap();
    demo_practical_use_cases();
    demo_hardware_mapping();
    demo_summary();
}

