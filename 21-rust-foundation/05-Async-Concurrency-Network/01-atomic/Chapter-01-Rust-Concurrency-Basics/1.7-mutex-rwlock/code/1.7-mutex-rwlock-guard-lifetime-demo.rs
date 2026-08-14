use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

/// 演示 MutexGuard 的生命周期：在代码块内存在，离开后自动销毁
fn demo_guard_lifetime() {
    println!("=== MutexGuard 的生命周期 ===");
    
    let mutex = Mutex::new(0);
    
    {
        // guard 在这个大括号内存在
        let mut guard = mutex.lock().unwrap();
        *guard += 10;
        println!("guard 存在，值: {}", *guard);
    } // 大括号结束，guard 被销毁，锁自动释放
    
    println!("guard 已销毁，锁已释放");
    
    // 现在可以再次获取锁
    let guard2 = mutex.lock().unwrap();
    println!("再次获取锁成功，值: {}", *guard2);
    
    println!();
}

/// 演示误区：在 if let 条件中直接使用 lock()，导致锁持有时间过长
fn demo_guard_lifetime_mistake() {
    println!("=== 误区：if let 中直接使用 lock() ===");
    
    let list = Arc::new(Mutex::new(vec![1, 2, 3]));
    
    // ❌ 错误写法：guard 会一直持有到整个 if let 代码块结束
    if let Some(item) = list.lock().unwrap().pop() {
        println!("弹出的元素: {}", item);
        // 假设这里有很多耗时操作
        thread::sleep(Duration::from_millis(100));
        println!("处理元素完成");
        // guard 直到这里才被销毁，锁才释放！
    }
    
    println!("问题：guard 在整个 if let 代码块内都存在");
    println!("即使我们只需要 pop() 操作，处理元素时锁还被占用");
    println!();
}

/// 演示正确做法：把 lock() 提取到单独作用域，尽早释放锁
fn demo_guard_lifetime_correct() {
    println!("=== 正确做法：提前释放锁 ===");
    
    let list = Arc::new(Mutex::new(vec![1, 2, 3]));
    
    // ✅ 正确写法：用大括号限定 guard 的作用域
    let item = {
        let mut guard = list.lock().unwrap(); // 获取锁
        guard.pop() // 取元素
    }; // 大括号结束，guard 立刻销毁，锁释放！
    
    // 现在处理元素，锁已经释放了，其他线程可以操作 list
    if let Some(item) = item {
        println!("弹出的元素: {}", item);
        thread::sleep(Duration::from_millis(100)); // 耗时操作不影响锁
        println!("处理元素完成（此时锁已释放）");
    }
    
    println!("优点：锁只在必要时持有，处理数据时不影响其他线程");
    println!();
}

/// 演示 match 和 while let 中的相同问题
fn demo_match_and_while_let() {
    println!("=== match 和 while let 中的生命周期 ===");
    
    let list = Mutex::new(vec![1, 2, 3]);
    
    // match 中：guard 会持有到整个 match 代码块结束
    match list.lock().unwrap().pop() {
        Some(item) => {
            println!("match 中弹出: {}", item);
            thread::sleep(Duration::from_millis(50));
            // guard 直到这里才释放
        }
        None => println!("列表为空"),
    }
    
    // while let 中：guard 会持有到整个循环体结束
    let list2 = Mutex::new(vec![1, 2, 3]);
    while let Some(item) = list2.lock().unwrap().pop() {
        println!("while let 中弹出: {}", item);
        thread::sleep(Duration::from_millis(50));
        // guard 每次循环结束才释放，下次循环开始又获取
    }
    
    println!("注意：match 和 while let 中，guard 的生命周期也是整个代码块");
    println!();
}

/// 演示多线程下 lock() 的阻塞行为
fn demo_lock_blocking() {
    println!("=== 多线程下 lock() 的阻塞行为 ===");
    
    let shared_data = Arc::new(Mutex::new(0));
    let data_clone1 = Arc::clone(&shared_data);
    let data_clone2 = Arc::clone(&shared_data);
    
    // 线程1：获取锁后休眠，模拟长时间持有锁
    let handle1 = thread::spawn(move || {
        println!("线程1: 尝试获取锁...");
        let mut guard = data_clone1.lock().unwrap();
        println!("线程1: 获取锁成功，持有锁 1 秒");
        *guard = 100;
        thread::sleep(Duration::from_secs(1));
        println!("线程1: 释放锁");
        // guard 离开作用域，锁释放
    });
    
    // 线程2：在线程1持有锁时尝试获取锁（会被阻塞）
    let handle2 = thread::spawn(move || {
        thread::sleep(Duration::from_millis(100)); // 稍微延迟，确保线程1先拿到锁
        println!("线程2: 尝试获取锁（线程1还在持有）...");
        let mut guard = data_clone2.lock().unwrap(); // 这里会阻塞等待
        println!("线程2: 获取锁成功（线程1已释放）");
        *guard = 200;
    });
    
    handle1.join().unwrap();
    handle2.join().unwrap();
    
    println!("最终值: {}", shared_data.lock().unwrap());
    println!("说明：lock() 阻塞是正常等待，不是错误，也不是锁中毒");
    println!();
}

/// 演示锁中毒的情况
fn demo_lock_poisoning() {
    println!("=== 锁中毒的情况 ===");
    
    let mutex = Arc::new(Mutex::new(0));
    let mutex_clone = Arc::clone(&mutex);
    
    // 线程1：持锁时 panic，导致锁中毒
    let handle1 = thread::spawn(move || {
        let mut guard = mutex_clone.lock().unwrap();
        *guard = 100;
        println!("线程1: 持有锁，准备 panic...");
        panic!("线程1 故意 panic，导致锁中毒");
    });
    
    // 等待线程1完成（panic）
    let _ = handle1.join(); // join() 返回 Err，因为线程 panic 了
    
    // 线程2：尝试获取中毒的锁
    let mutex_clone2 = Arc::clone(&mutex);
    let handle2 = thread::spawn(move || {
        println!("线程2: 尝试获取锁（锁已中毒）...");
        
        match mutex_clone2.lock() {
            Ok(mut guard) => {
                // 正常情况（虽然锁中毒，但 into_inner() 后仍可获取）
                println!("线程2: 获取锁成功（锁曾中毒但已恢复）");
                *guard = 200;
            }
            Err(poisoned) => {
                // 锁中毒的情况
                println!("线程2: 检测到锁中毒！");
                let mut guard = poisoned.into_inner(); // 强行获取锁
                *guard = 200;
                println!("线程2: 强行获取锁，继续操作");
            }
        }
    });
    
    handle2.join().unwrap();
    
    // 主线程也需要处理锁中毒的情况
    match mutex.lock() {
        Ok(guard) => println!("最终值: {}", guard),
        Err(poisoned) => {
            let guard = poisoned.into_inner();
            println!("最终值: {} (锁曾中毒但已恢复)", guard);
        }
    }
    println!("说明：锁中毒后，lock() 返回 Err(PoisonError)，需要手动处理");
    println!();
}

/// 演示最佳实践：正确处理锁和错误
fn demo_best_practice() {
    println!("=== 最佳实践：正确处理锁和错误 ===");
    
    let list = Arc::new(Mutex::new(vec![1, 2, 3, 4, 5]));
    let mut handles = vec![];
    
    for i in 0..3 {
        let list_clone = Arc::clone(&list);
        
        let handle = thread::spawn(move || {
            // 第一步：在单独作用域中获取锁并操作数据
            let item = {
                // 处理 lock() 可能的错误（锁中毒）
                match list_clone.lock() {
                    Ok(mut guard) => guard.pop(),
                    Err(poisoned) => {
                        println!("线程 {}: 检测到锁中毒，强行获取", i);
                        poisoned.into_inner().pop()
                    }
                }
            }; // guard 在这里销毁，锁释放
            
            // 第二步：处理取出的数据（此时锁已释放）
            if let Some(item) = item {
                println!("线程 {}: 弹出元素 {}，开始处理（锁已释放）", i, item);
                thread::sleep(Duration::from_millis(100)); // 耗时操作不影响锁
                println!("线程 {}: 处理完成", i);
            } else {
                println!("线程 {}: 列表为空", i);
            }
        });
        
        handles.push(handle);
    }
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!("优点：");
    println!("  1. 锁只在必要时持有（取数据时）");
    println!("  2. 处理数据时锁已释放，其他线程可以操作");
    println!("  3. 正确处理了锁中毒的情况");
    println!();
}

/// 总结 MutexGuard 生命周期的关键点
fn demo_summary() {
    println!("=== MutexGuard 生命周期总结 ===");
    println!();
    println!("1. 生命周期规则：");
    println!("   - MutexGuard 在代码块内存在");
    println!("   - 离开代码块时自动销毁（Drop trait）");
    println!("   - 销毁时自动释放锁");
    println!();
    println!("2. 常见误区：");
    println!("   - 在 if let/match/while let 条件中直接使用 lock()");
    println!("   - guard 会持有到整个代码块结束");
    println!("   - 导致处理数据时还占用锁，影响其他线程");
    println!();
    println!("3. 正确做法：");
    println!("   - 把 lock() 提取到单独的作用域（大括号）");
    println!("   - 取完数据后立刻释放锁");
    println!("   - 在锁外处理数据，不影响其他线程");
    println!();
    println!("4. 多线程行为：");
    println!("   - lock() 阻塞是正常等待，不是错误");
    println!("   - 只有锁中毒时，lock() 才返回 Err(PoisonError)");
    println!("   - 应该用 match/if let 处理可能的错误");
    println!();
}

pub fn main() {
    demo_guard_lifetime();
    demo_guard_lifetime_mistake();
    demo_guard_lifetime_correct();
    demo_match_and_while_let();
    demo_lock_blocking();
    demo_lock_poisoning();
    demo_best_practice();
    demo_summary();
}

