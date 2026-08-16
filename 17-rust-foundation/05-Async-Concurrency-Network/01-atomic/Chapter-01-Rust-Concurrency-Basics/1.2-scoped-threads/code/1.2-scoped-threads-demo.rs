use std::thread;
use std::sync::{Arc, Mutex};

// thread::scope 详解
// 
// 核心特点：
// 1. scope 会阻塞主线程，直到所有子线程执行完毕（相当于自动批量 join）
// 2. scope 外部的变量可以直接借用，不需要 move
// 3. scope 内部的变量如果要在 spawn 中使用，通常需要 move
// 4. 子线程之间执行顺序是随机的（由操作系统调度）
// 
// 如何在 scope 外部使用 scope 内部的数据？
// 方法1：通过返回值（scope 可以返回值）
// 方法2：使用共享数据结构（Arc + Mutex）
// 方法3：将数据定义在 scope 外部，在 scope 内部修改

// 示例：thread::scope 的基本用法
pub fn main() {
    println!("=== thread::scope 示例 ===\n");
    
    // 示例 1: scope 的基本用法，不需要 move
    example1_basic_scope();
    
    // 示例 2: scope 内多个线程的执行顺序
    example2_multiple_threads();
    
    // 示例 3: 如何在 scope 外部使用 scope 内部的数据（方法1：返回值）
    example3_return_value();
    
    // 示例 4: 如何在 scope 外部使用 scope 内部的数据（方法2：共享数据）
    example4_shared_data();
    
    // 示例 5: scope 内外的变量访问规则
    example5_variable_access();
}

// 示例 1: scope 的基本用法，不需要 move 就能借用外部变量
fn example1_basic_scope() {
    println!("--- 示例 1: scope 基本用法（不需要 move）---");
    
    // scope 外部的变量
    let outer_var = 100;
    let vec = vec![1, 2, 3, 4, 5];
    
    // thread::scope 会阻塞，直到所有子线程执行完毕
    thread::scope(|s| {
        // scope 内部的变量（如果要在 spawn 中使用，通常需要 move）
        let inner_var = 200;
        
        // 不需要 move！可以直接借用 scope 外部的变量
        // 但 scope 内部的变量通常需要 move（除非是引用）
        s.spawn(|| {
            println!("子线程使用外部变量: {}", outer_var);
            println!("子线程使用外部 vec: {:?}", vec);
        });
        
        // 如果要用内部变量，需要 move
        s.spawn(move || {
            println!("子线程使用内部变量: {}", inner_var);
        });
        
        // scope 内的主线程代码也会执行
        println!("scope 内的主线程代码");
    });
    
    // scope 执行完后，主线程继续
    println!("scope 外的代码（所有子线程跑完才执行）\n");
}

// 示例 2: scope 内多个线程的执行顺序（随机）
fn example2_multiple_threads() {
    println!("--- 示例 2: 多个线程的执行顺序（随机）---");
    
    thread::scope(|s| {
        s.spawn(|| {
            println!("子线程1开始");
            thread::sleep(std::time::Duration::from_millis(100));
            println!("子线程1结束");
        });
        
        s.spawn(|| {
            println!("子线程2开始");
            thread::sleep(std::time::Duration::from_millis(50));
            println!("子线程2结束");
        });
        
        s.spawn(|| {
            println!("子线程3开始");
            thread::sleep(std::time::Duration::from_millis(75));
            println!("子线程3结束");
        });
        
        println!("scope 内的主线程代码（先执行）");
    });
    
    println!("scope 外的代码（所有子线程跑完才执行）\n");
}

// 示例 3: 方法1 - 通过返回值在 scope 外部使用 scope 内部的数据
fn example3_return_value() {
    println!("--- 示例 3: 通过返回值在 scope 外部使用数据 ---");
    
    // 方法1a: scope 可以返回值（最简单的方式）
    let result = thread::scope(|s| {
        let numbers = vec![1, 2, 3, 4, 5];
        let mut results = Vec::new();
        
        // 在 scope 内部计算结果
        let sum: i32 = numbers.iter().sum();
        let len = numbers.len();
        let avg = sum as f64 / len as f64;
        
        // 也可以让子线程计算结果
        s.spawn(move || {
            println!("子线程计算: sum={}, len={}, avg={:.2}", sum, len, avg);
        });
        
        // scope 返回计算结果
        results.push(sum);
        results.push(len as i32);
        results.push(avg as i32);
        results  // 返回值
    });
    
    // scope 外部可以直接使用返回值
    println!("scope 返回的结果: {:?}", result);
    
    // 方法1b: 使用共享数据结构（Arc + Mutex）
    let shared_result = Arc::new(Mutex::new(Vec::new()));
    let shared_clone = Arc::clone(&shared_result);
    
    thread::scope(|s| {
        let numbers = vec![10, 20, 30];
        
        s.spawn(move || {
            let sum: i32 = numbers.iter().sum();
            let mut data = shared_clone.lock().unwrap();
            data.push(sum);
        });
    });
    
    let final_result = shared_result.lock().unwrap();
    println!("通过共享数据获取的结果: {:?}", *final_result);
    println!();
}

// 示例 4: 方法2 - 使用共享数据结构（Arc + Mutex）
fn example4_shared_data() {
    println!("--- 示例 4: 使用共享数据结构（Arc + Mutex）---");
    
    // 在 scope 外部创建共享数据
    let shared_data = Arc::new(Mutex::new(0));
    
    thread::scope(|s| {
        // 创建多个线程，每个线程修改共享数据
        for i in 1..=5 {
            let data = Arc::clone(&shared_data);
            s.spawn(move || {
                let mut num = data.lock().unwrap();
                *num += i;
                println!("线程 {} 将共享数据增加 {}", i, i);
            });
        }
    });
    
    // scope 外部可以访问最终结果
    let final_value = shared_data.lock().unwrap();
    println!("scope 外部获取的最终值: {}", *final_value);
    println!();
}

// 示例 5: scope 内外的变量访问规则
fn example5_variable_access() {
    println!("--- 示例 5: scope 内外的变量访问规则 ---");
    
    // ✅ 可以：scope 上面的变量
    let before_scope = 10;
    
    // 方法1: 通过返回值传递数据
    let result = thread::scope(|s| {
        // ✅ 可以：scope 内部的变量
        let inside_scope = 20;
        let mut results = Vec::new();
        
        // 使用 scope 上面的变量，不需要 move
        s.spawn(|| {
            println!("使用 scope 上面的变量: {}", before_scope);
        });
        
        // 使用 scope 内部的变量，需要 move
        s.spawn(move || {
            println!("使用 scope 内部的变量: {}", inside_scope);
        });
        
        // 在 scope 内部计算结果并返回
        results.push(before_scope + inside_scope);
        results
    });
    
    // ✅ 可以：使用 scope 返回的值
    println!("scope 返回的结果: {:?}", result);
    
    // ❌ 不能：直接使用 scope 内部定义的变量（作用域限制）
    // println!("{}", inside_scope); // 编译错误！
    
    // ✅ 可以：scope 下面的变量（在 scope 执行完后定义）
    let after_scope = 30;
    println!("scope 下面的变量: {}\n", after_scope);
}

// 额外示例：对比普通 spawn 和 scope 的区别
#[allow(dead_code)]
fn compare_spawn_vs_scope() {
    println!("--- 对比：普通 spawn vs scope ---");
    
    let vec = vec![1, 2, 3];
    
    // 普通 spawn：需要 move，所有权转移
    let handle = thread::spawn(move || {
        println!("普通 spawn: {:?}", vec);
    });
    handle.join().unwrap();
    // 注意：这里不能再使用 vec，因为所有权已经转移
    
    // scope：不需要 move，可以借用
    let vec2 = vec![4, 5, 6];
    thread::scope(|s| {
        s.spawn(|| {
            println!("scope: {:?}", vec2); // 直接借用，不需要 move
        });
    });
    // 注意：scope 执行完后，vec2 仍然可以使用（如果没被 move）
}

