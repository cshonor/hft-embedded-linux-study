use std::sync::atomic::{AtomicU32, Ordering};
use std::thread;
use std::time::Duration;

/// 方法1：简单的 ID 分配器（使用 fetch_add）
fn allocate_id_simple() -> u32 {
    static NEXT_ID: AtomicU32 = AtomicU32::new(0);
    // fetch_add 返回旧值，然后原子地加 1
    NEXT_ID.fetch_add(1, Ordering::Relaxed)
}

/// 方法2：带溢出检查的 ID 分配器（使用 fetch_sub 回滚）
fn allocate_id_with_rollback() -> u32 {
    static NEXT_ID: AtomicU32 = AtomicU32::new(0);
    
    let id = NEXT_ID.fetch_add(1, Ordering::Relaxed);
    
    // 假设阈值是 1000（实际应该用 u32::MAX）
    if id >= 1000 {
        // 用 fetch_sub 把加的 1 减回去（回滚）
        NEXT_ID.fetch_sub(1, Ordering::Relaxed);
        panic!("ID 溢出！已分配 {} 个 ID", id);
    }
    
    id
}

/// 方法3：使用 compare_exchange 的 ID 分配器（支持循环复用）
fn allocate_id_with_cas() -> u32 {
    static NEXT_ID: AtomicU32 = AtomicU32::new(0);
    
    loop {
        // ① 加载当前值
        let current = NEXT_ID.load(Ordering::Relaxed);
        
        // ② 计算新值：如果快溢出，重置为 0；否则加 1
        let next = if current >= u32::MAX - 1 {
            0  // 溢出后重置为 0（循环复用）
        } else {
            current + 1
        };
        
        // ③ CAS 操作：只有当 NEXT_ID 还是 current 时，才更新为 next
        match NEXT_ID.compare_exchange(
            current,          // 预期值：我认为现在 NEXT_ID 还是 current
            next,             // 新值：如果预期对，就改成 next
            Ordering::Relaxed,// 成功时的内存顺序
            Ordering::Relaxed // 失败时的内存顺序
        ) {
            Ok(_) => return current, // CAS 成功！返回当前 ID
            Err(v) => {
                // CAS 失败：其他线程修改了 NEXT_ID，v 是最新值
                // 继续循环，用最新值重新尝试
                continue;
            }
        }
    }
}

/// 演示简单的 ID 分配器
fn demo_simple_allocator() {
    println!("=== 方法1：简单的 ID 分配器（fetch_add）===");
    
    for i in 0..5 {
        let id = allocate_id_simple();
        println!("第 {} 次调用，分配 ID: {}", i + 1, id);
    }
    
    println!("说明：fetch_add 返回旧值，然后原子地加 1");
    println!("      每次调用返回的 ID 都是唯一的，从 0 开始递增");
    println!();
}

/// 演示带溢出检查的 ID 分配器
fn demo_allocator_with_rollback() {
    println!("=== 方法2：带溢出检查的 ID 分配器（fetch_sub 回滚）===");
    
    // 重置静态变量（在实际场景中，static 变量无法重置，这里只是演示）
    println!("分配前 5 个 ID：");
    for i in 0..5 {
        let id = allocate_id_with_rollback();
        println!("第 {} 次调用，分配 ID: {}", i + 1, id);
    }
    
    println!("说明：当 ID >= 1000 时，用 fetch_sub 回滚，然后 panic");
    println!("      这样可以避免 ID 继续增长，但会终止程序");
    println!("注意：实际代码中 static 变量无法重置，这里只是演示逻辑");
    println!();
}

/// 演示使用 compare_exchange 的 ID 分配器
fn demo_allocator_with_cas() {
    println!("=== 方法3：使用 compare_exchange 的 ID 分配器 ===");
    
    println!("分配前 5 个 ID：");
    for i in 0..5 {
        let id = allocate_id_with_cas();
        println!("第 {} 次调用，分配 ID: {}", i + 1, id);
    }
    
    println!("说明：compare_exchange 可以自定义逻辑（如溢出后重置为 0）");
    println!("      即使多线程同时调用，也能保证 ID 唯一且不重复");
    println!();
}

/// 演示多线程下的 ID 分配
fn demo_multithread_allocator() {
    println!("=== 多线程下的 ID 分配 ===");
    
    let mut handles = vec![];
    
    // 创建 3 个线程，每个线程分配 5 个 ID
    for i in 0..3 {
        let handle = thread::spawn(move || {
            let mut ids = vec![];
            for _ in 0..5 {
                let id = allocate_id_simple();
                ids.push(id);
                thread::sleep(Duration::from_millis(10));
            }
            println!("线程 {}: 分配的 ID: {:?}", i, ids);
            ids
        });
        handles.push(handle);
    }
    
    let mut all_ids = vec![];
    for handle in handles {
        let ids = handle.join().unwrap();
        all_ids.extend(ids);
    }
    
    all_ids.sort();
    println!("所有分配的 ID: {:?}", all_ids);
    println!("说明：即使多线程同时调用，每个 ID 都是唯一的");
    println!("      fetch_add 的原子性保证了不会出现重复 ID");
    println!();
}

/// 演示 compare_exchange 的工作原理
fn demo_compare_exchange_principle() {
    println!("=== compare_exchange 工作原理 ===");
    
    static COUNTER: AtomicU32 = AtomicU32::new(10);
    
    println!("初始值: {}", COUNTER.load(Ordering::Relaxed));
    
    // 场景1：预期值匹配，成功更新
    let result1 = COUNTER.compare_exchange(
        10,              // 预期值：我认为现在是 10
        20,              // 新值：如果预期对，改成 20
        Ordering::Relaxed,
        Ordering::Relaxed
    );
    println!("场景1: compare_exchange(10, 20) = {:?}", result1);
    println!("  当前值: {}", COUNTER.load(Ordering::Relaxed));
    
    // 场景2：预期值不匹配，失败
    let result2 = COUNTER.compare_exchange(
        10,              // 预期值：我认为现在是 10（但实际是 20）
        30,              // 新值：想改成 30
        Ordering::Relaxed,
        Ordering::Relaxed
    );
    println!("场景2: compare_exchange(10, 30) = {:?}", result2);
    println!("  当前值: {}", COUNTER.load(Ordering::Relaxed));
    
    // 场景3：预期值匹配，成功更新
    let result3 = COUNTER.compare_exchange(
        20,              // 预期值：我认为现在是 20
        30,              // 新值：如果预期对，改成 30
        Ordering::Relaxed,
        Ordering::Relaxed
    );
    println!("场景3: compare_exchange(20, 30) = {:?}", result3);
    println!("  当前值: {}", COUNTER.load(Ordering::Relaxed));
    
    println!();
    println!("说明：");
    println!("  - Ok(old_value)：CAS 成功，返回旧值");
    println!("  - Err(current_value)：CAS 失败，返回当前实际值");
    println!("  - CAS 是原子操作，不会被其他线程打断");
    println!();
}

/// 演示用 compare_exchange 实现原子自增（模拟 fetch_add）
fn demo_cas_increment() {
    println!("=== 用 compare_exchange 实现原子自增 ===");
    
    static COUNTER: AtomicU32 = AtomicU32::new(0);
    
    fn increment(a: &AtomicU32) {
        let mut current = a.load(Ordering::Relaxed);
        loop {
            let new = current + 1;
            match a.compare_exchange(current, new, Ordering::Relaxed, Ordering::Relaxed) {
                Ok(_) => return,      // CAS 成功，值已更新
                Err(v) => current = v, // CAS 失败，更新 current 为最新值，重新尝试
            }
        }
    }
    
    println!("初始值: {}", COUNTER.load(Ordering::Relaxed));
    
    increment(&COUNTER);
    println!("调用 increment() 后: {}", COUNTER.load(Ordering::Relaxed));
    
    increment(&COUNTER);
    println!("再次调用 increment() 后: {}", COUNTER.load(Ordering::Relaxed));
    
    println!("说明：compare_exchange 可以实现任意自定义的原子逻辑");
    println!("      fetch_add 底层就是用类似的方式实现的");
    println!();
}

/// 演示原子类型的引用可以执行写操作
fn demo_atomic_reference_write() {
    println!("=== 原子类型的引用可以执行写操作 ===");
    
    let atomic = AtomicU32::new(0);
    let atomic_ref: &AtomicU32 = &atomic; // 不可变引用
    
    println!("初始值: {}", atomic_ref.load(Ordering::Relaxed));
    
    // 通过不可变引用调用写操作（fetch_add）
    let old = atomic_ref.fetch_add(10, Ordering::Relaxed);
    println!("通过不可变引用调用 fetch_add(10)");
    println!("  返回旧值: {}, 当前值: {}", old, atomic_ref.load(Ordering::Relaxed));
    
    // 通过不可变引用调用 store
    atomic_ref.store(100, Ordering::Relaxed);
    println!("通过不可变引用调用 store(100)");
    println!("  当前值: {}", atomic_ref.load(Ordering::Relaxed));
    
    println!("说明：原子类型的不可变引用（&AtomicU32）可以调用写操作");
    println!("      因为原子操作的线程安全不依赖引用的可变性");
    println!("      而是依赖硬件级别的原子指令");
    println!();
}

/// 总结三种解决 ID 溢出的方法
fn demo_overflow_solutions_summary() {
    println!("=== ID 溢出的三种解决方法总结 ===");
    println!();
    println!("方法1：终止进程（abort）");
    println!("  - 使用 std::process::abort()");
    println!("  - 优点：简单直接");
    println!("  - 缺点：会终止整个程序，用户体验差");
    println!("  - 适用：ID 溢出会导致严重问题的场景");
    println!();
    println!("方法2：fetch_sub 回滚 + panic");
    println!("  - 检测到溢出时，用 fetch_sub 把值减回去");
    println!("  - 然后触发 panic");
    println!("  - 优点：避免 ID 继续增长");
    println!("  - 缺点：会终止线程/程序");
    println!("  - 适用：需要严格限制 ID 范围的场景");
    println!();
    println!("方法3：compare_exchange（CAS）");
    println!("  - 用 CAS 实现自定义逻辑（如溢出后重置为 0）");
    println!("  - 失败时循环重试");
    println!("  - 优点：灵活，不终止程序，支持循环复用");
    println!("  - 缺点：代码稍复杂");
    println!("  - 适用：生产环境推荐方案");
    println!();
}

/// 演示为什么 fetch_add 返回旧值
fn demo_why_fetch_add_returns_old_value() {
    println!("=== 为什么 fetch_add 返回旧值？===");
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
    demo_simple_allocator();
    demo_allocator_with_rollback();
    demo_allocator_with_cas();
    demo_multithread_allocator();
    demo_compare_exchange_principle();
    demo_cas_increment();
    demo_atomic_reference_write();
    demo_overflow_solutions_summary();
    demo_why_fetch_add_returns_old_value();
}

