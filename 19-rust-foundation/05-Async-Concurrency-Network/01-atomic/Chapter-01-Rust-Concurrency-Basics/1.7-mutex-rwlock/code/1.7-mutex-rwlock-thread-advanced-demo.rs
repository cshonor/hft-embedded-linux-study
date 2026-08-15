use std::thread;
use std::sync::{Arc, Mutex};
use std::time::Duration;

// Rust 线程高级用法示例
// 
// 涵盖内容：
// 1. 多线程同时修改共享数据（需要 Mutex）
// 2. 多线程只读共享数据（不需要锁）
// 3. Box::leak 和静态引用的使用
// 4. Arc + Mutex 的完整应用场景

pub fn main() {
    println!("=== Rust 线程高级用法示例 ===\n");
    
    // 示例 1: 多线程同时 push（会报错的情况）
    example1_multiple_push_error();
    
    // 示例 2: 使用 Mutex 解决多线程 push 问题
    example2_multiple_push_with_mutex();
    
    // 示例 3: 多线程只读（不需要锁）
    example3_multiple_read();
    
    // 示例 4: Box::leak 和静态引用
    example4_box_leak();
    
    // 示例 5: 实际应用场景 - 多线程计算并汇总
    example5_parallel_computation();
}

// 示例 1: 多线程同时 push 会报错（演示问题）
fn example1_multiple_push_error() {
    println!("--- 示例 1: 多线程同时 push 会报错 ---");
    
    // 注意：这段代码会编译报错，这里只是演示问题
    println!("如果直接让多个线程同时 push，会报错：");
    println!("error: cannot borrow `vec` as mutable more than once");
    println!();
    
    // 注释掉的错误代码：
    /*
    let mut vec = vec![1, 2, 3];
    thread::scope(|s| {
        s.spawn(|| {
            vec.push(4); // ❌ 错误：可变借用冲突
        });
        s.spawn(|| {
            vec.push(5); // ❌ 错误：可变借用冲突
        });
    });
    */
    
    println!("原因：Rust 不允许同一时间有多个可变借用\n");
}

// 示例 2: 使用 Mutex 解决多线程 push 问题
fn example2_multiple_push_with_mutex() {
    println!("--- 示例 2: 使用 Mutex 解决多线程 push ---");
    
    // 在 scope 外部创建共享数据（用 Arc 包装 Mutex）
    let shared_vec = Arc::new(Mutex::new(vec![1, 2, 3]));
    
    thread::scope(|s| {
        // 创建多个线程，每个线程都 push 数据
        for i in 4..=7 {
            let vec_clone = Arc::clone(&shared_vec);
            s.spawn(move || {
                // 加锁获取可变访问权
                let mut guard = vec_clone.lock().unwrap();
                guard.push(i);
                println!("线程 {} push 了 {}", i, i);
                // guard 离开作用域时自动释放锁
            });
        }
    });
    
    // scope 外部可以访问最终结果
    let final_vec = shared_vec.lock().unwrap();
    println!("最终数组: {:?}", *final_vec);
    println!("注意：顺序可能不同，因为线程执行是并发的\n");
}

// 示例 3: 多线程只读（不需要锁）
fn example3_multiple_read() {
    println!("--- 示例 3: 多线程只读（不需要锁）---");
    
    // 定义数组（可以是 mut，但子线程只读）
    let arr = [10, 20, 30, 40, 50];
    
    thread::scope(|s| {
        // 创建多个线程，每个线程读取不同的元素
        for i in 0..arr.len() {
            s.spawn(move || {
                println!("线程 {} 读取: arr[{}] = {}", i, i, arr[i]);
            });
        }
    });
    
    println!("多个线程同时读完全安全，不需要加锁！\n");
}

// 示例 4: Box::leak 和静态引用
fn example4_box_leak() {
    println!("--- 示例 4: Box::leak 和静态引用 ---");
    
    // 方法 1: Box::leak 返回可变引用（需要转成不可变才能多线程共享）
    let x_mut: &'static mut [i32; 3] = Box::leak(Box::new([1, 2, 3]));
    
    // 转成不可变引用，才能被多个线程共享
    let x: &'static [i32; 3] = x_mut;
    
    thread::scope(|s| {
        // 多个线程可以同时读取静态不可变引用
        s.spawn(|| {
            println!("线程1读取: {:?}", x);
        });
        s.spawn(|| {
            println!("线程2读取: {:?}", x);
        });
        s.spawn(|| {
            println!("线程3读取: {:?}", x);
        });
    });
    
    println!("Box::leak 的数据会一直存在到程序结束");
    println!("不可变静态引用可以被多个线程安全共享\n");
    
    // 方法 2: 如果需要修改，用 Mutex 包装
    let x_mutex: &'static Mutex<[i32; 3]> = Box::leak(Box::new(Mutex::new([10, 20, 30])));
    
    thread::scope(|s| {
        s.spawn(|| {
            let mut guard = x_mutex.lock().unwrap();
            guard[0] = 100;
            println!("线程修改: {:?}", *guard);
        });
        
        s.spawn(|| {
            let guard = x_mutex.lock().unwrap();
            println!("线程读取: {:?}", *guard);
        });
    });
    
    println!();
}

// 示例 5: 实际应用场景 - 多线程并行计算并汇总结果
fn example5_parallel_computation() {
    println!("--- 示例 5: 多线程并行计算并汇总 ---");
    
    let numbers: Vec<i32> = (1..=100).collect();
    let shared_sum = Arc::new(Mutex::new(0));
    
    // 将数组分成 4 段，每段用一个线程计算
    let chunk_size = numbers.len() / 4;
    
    thread::scope(|s| {
        for chunk in numbers.chunks(chunk_size) {
            let chunk_vec = chunk.to_vec();
            let sum_clone = Arc::clone(&shared_sum);
            
            s.spawn(move || {
                // 计算这一段的平方和
                let chunk_sum: i32 = chunk_vec.iter().map(|x| x * x).sum();
                
                // 将结果累加到共享变量
                let mut total = sum_clone.lock().unwrap();
                *total += chunk_sum;
                println!("线程计算了 {} 个数的平方和: {}", chunk_vec.len(), chunk_sum);
            });
        }
    });
    
    let final_sum = shared_sum.lock().unwrap();
    println!("所有线程计算的总和: {}", *final_sum);
    
    // 验证：直接计算应该得到相同结果
    let expected: i32 = (1..=100).map(|x| x * x).sum();
    println!("验证（直接计算）: {}", expected);
    println!("结果匹配: {}\n", *final_sum == expected);
}

// 额外示例：对比不同共享方式的性能
#[allow(dead_code)]
fn compare_shared_methods() {
    println!("--- 对比：不同共享方式 ---");
    
    // 方式 1: Arc<Mutex<T>> - 适合需要修改的场景
    let data1 = Arc::new(Mutex::new(0));
    let data1_clone = Arc::clone(&data1);
    
    thread::scope(|s| {
        s.spawn(move || {
            let mut guard = data1_clone.lock().unwrap();
            *guard += 1;
        });
    });
    
    // 方式 2: 静态不可变引用 - 适合只读场景
    static STATIC_DATA: [i32; 3] = [1, 2, 3];
    thread::scope(|s| {
        s.spawn(|| {
            println!("读取静态数据: {:?}", STATIC_DATA);
        });
    });
    
    // 方式 3: Box::leak + 不可变引用 - 适合运行时创建的只读数据
    let leaked: &'static [i32] = Box::leak(Box::new([4, 5, 6]));
    thread::scope(|s| {
        s.spawn(|| {
            println!("读取泄漏数据: {:?}", leaked);
        });
    });
}

