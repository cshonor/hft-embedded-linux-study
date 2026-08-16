use std::thread;

pub fn main() {
    println!("=== Rust 线程示例 ===\n");

    // 示例 1: 基本的线程创建和使用 join
    example1_basic_thread();
    
    // 示例 2: 使用 move 转移所有权
    example2_move_ownership();
    
    // 示例 3: 多个线程和 Vec
    example3_multiple_threads();
    
    // 示例 4: 线程间传递数据
    example4_thread_with_data();
}

// 示例 1: 基本的线程创建，使用 join 等待线程完成
fn example1_basic_thread() {
    println!("--- 示例 1: 基本线程创建 ---");
    
    let t1 = thread::spawn(|| {
        println!("子线程 t1 正在运行");
        for i in 1..=3 {
            println!("t1: {}", i);
            thread::sleep(std::time::Duration::from_millis(100));
        }
    });
    
    // t1.join() 会阻塞主线程，直到 t1 执行完毕
    t1.join().unwrap();
    println!("t1 执行完毕，主线程继续\n");
}

// 示例 2: 使用 move 转移所有权
fn example2_move_ownership() {
    println!("--- 示例 2: move 关键字转移所有权 ---");
    
    let vec = vec![1, 2, 3];
    println!("主线程创建了 vec: {:?}", vec);
    
    // 使用 move 将 vec 的所有权转移到子线程
    let t1 = thread::spawn(move || {
        println!("子线程接收到 vec: {:?}", vec);
        // 计算 vec 的长度和总和
        let len = vec.len();
        let sum: i32 = vec.iter().sum();
        println!("vec 长度: {}, 总和: {}", len, sum);
    });
    
    // 注意：这里不能再使用 vec，因为所有权已经转移给子线程了
    // println!("{:?}", vec); // 这行会编译错误！
    
    t1.join().unwrap();
    println!("示例 2 完成\n");
}

// 示例 3: 创建多个线程，演示并发执行
fn example3_multiple_threads() {
    println!("--- 示例 3: 多个线程并发执行 ---");
    
    let vec = vec![10, 20, 30, 40, 50];
    
    // 创建多个线程，每个线程处理 vec 的一部分
    let t1 = thread::spawn(move || {
        println!("线程 t1 开始处理");
        for item in vec.iter() {
            println!("t1 处理: {}", item);
            thread::sleep(std::time::Duration::from_millis(50));
        }
    });
    
    let t2 = thread::spawn(|| {
        println!("线程 t2 开始处理");
        for i in 1..=5 {
            println!("t2 处理: {}", i * 100);
            thread::sleep(std::time::Duration::from_millis(50));
        }
    });
    
    // 等待两个线程都完成
    t1.join().unwrap();
    t2.join().unwrap();
    println!("示例 3 完成\n");
}

// 示例 4: 使用 Vec::from_iter 和范围语法
fn example4_thread_with_data() {
    println!("--- 示例 4: 使用 Vec::from_iter 和范围 ---");
    
    // 使用范围语法创建迭代器，然后转成 Vec
    let numbers: Vec<i32> = (1..=10).collect();
    // 或者使用 Vec::from_iter
    // let numbers = Vec::from_iter(1..=10);
    
    println!("创建的 numbers: {:?}", numbers);
    println!("numbers 长度: {}", numbers.len());
    println!("numbers 总和: {}", numbers.iter().sum::<i32>());
    
    // 将 Vec 的所有权转移给线程
    let t1 = thread::spawn(move || {
        println!("子线程计算 numbers 的统计信息:");
        let len = numbers.len();
        let sum: i32 = numbers.iter().sum();
        let avg = sum as f64 / len as f64;
        println!("  长度: {}", len);
        println!("  总和: {}", sum);
        println!("  平均值: {:.2}", avg);
    });
    
    t1.join().unwrap();
    println!("示例 4 完成\n");
}

