use std::sync::{Arc, Condvar, Mutex};
use std::thread;
use std::time::Duration;
use std::collections::VecDeque;

/// 演示 Condvar 的基本使用：线程同步协调
fn demo_condvar_basic() {
    println!("=== Condvar 基本使用：线程同步协调 ===");
    
    // Condvar 解决的是同步问题（协调执行顺序）
    // Mutex 解决的是互斥问题（避免数据竞争）
    
    struct TaskQueue {
        queue: Mutex<VecDeque<u32>>,
        condvar: Condvar,
    }
    
    let task_queue = Arc::new(TaskQueue {
        queue: Mutex::new(VecDeque::new()),
        condvar: Condvar::new(),
    });
    
    let consumer = Arc::clone(&task_queue);
    let producer = Arc::clone(&task_queue);
    
    // 消费者线程：等待队列有数据
    let consumer_handle = thread::spawn(move || {
        loop {
            // 1. 获取锁
            let mut q = consumer.queue.lock().unwrap();
            
            // 2. 检查条件（用 while 防止虚假唤醒）
            while q.is_empty() {
                // 3. wait 会自动释放锁，进入休眠，唤醒后重新获取锁
                q = consumer.condvar.wait(q).unwrap();
            }
            
            // 4. 条件满足，取出任务
            let item = q.pop_front().unwrap();
            drop(q); // 主动释放锁，让生产者能继续添加任务
            
            println!("消费者处理任务: {}", item);
            
            if item == 9 {
                break; // 处理完所有任务后退出
            }
        }
    });
    
    // 生产者线程：往队列添加数据
    let producer_handle = thread::spawn(move || {
        for i in 0..10 {
            thread::sleep(Duration::from_millis(500));
            
            // 1. 获取锁，添加任务
            let mut q = producer.queue.lock().unwrap();
            q.push_back(i);
            drop(q); // 释放锁
            
            // 2. 通知等待的消费者（唤醒一个）
            producer.condvar.notify_one();
            println!("生产者添加任务: {}", i);
        }
    });
    
    producer_handle.join().unwrap();
    consumer_handle.join().unwrap();
    
    println!("说明：Condvar 协调执行顺序，Mutex 保证数据安全");
    println!();
}

/// 演示 wait 和 park 的区别
fn demo_wait_vs_park() {
    println!("=== wait 和 park 的区别 ===");
    
    println!("Condvar::wait:");
    println!("  - 必须配合 Mutex 使用");
    println!("  - 会自动释放 Mutex 锁");
    println!("  - 唤醒后自动重新获取锁");
    println!("  - 用于等待特定条件满足");
    println!();
    
    println!("thread::park:");
    println!("  - 不依赖锁");
    println!("  - 只是让线程休眠");
    println!("  - 需要其他线程调用 unpark 唤醒");
    println!("  - 用于简单的线程暂停/恢复");
    println!();
}

/// 演示 wait_timeout：带超时的等待
fn demo_wait_timeout() {
    println!("=== wait_timeout：带超时的等待 ===");
    
    struct TaskQueue {
        queue: Mutex<VecDeque<u32>>,
        condvar: Condvar,
    }
    
    let task_queue = Arc::new(TaskQueue {
        queue: Mutex::new(VecDeque::new()),
        condvar: Condvar::new(),
    });
    
    let consumer = Arc::clone(&task_queue);
    
    let handle = thread::spawn(move || {
        let mut q = consumer.queue.lock().unwrap();
        
        // wait_timeout 会等待条件满足，或者超时
        let timeout = Duration::from_secs(2);
        let result = consumer.condvar.wait_timeout(q, timeout).unwrap();
        
        q = result.0; // 重新获取的锁守卫
        let timed_out = result.1.timed_out(); // 是否超时
        
        if timed_out {
            println!("等待超时！2秒内没有收到通知");
            println!("说明：超时后会自动唤醒，返回超时标志");
        } else {
            println!("被正常唤醒，队列有数据");
            if let Some(item) = q.pop_front() {
                println!("处理任务: {}", item);
            }
        }
    });
    
    // 不发送通知，让消费者超时
    thread::sleep(Duration::from_millis(100));
    println!("主线程：不发送通知，等待超时...");
    
    handle.join().unwrap();
    
    println!("关键点：超时后会自动唤醒，不会一直等待");
    println!();
}

/// 演示 park_timeout：带超时的线程休眠
fn demo_park_timeout() {
    println!("=== park_timeout：带超时的线程休眠 ===");
    
    let handle = thread::spawn(|| {
        println!("线程开始休眠（带超时 2 秒）");
        
        // park_timeout 会让线程休眠指定时间，或等待 unpark
        thread::park_timeout(Duration::from_secs(2));
        
        println!("线程被唤醒（可能是超时，也可能是 unpark）");
    });
    
    // 不调用 unpark，让线程超时自动唤醒
    thread::sleep(Duration::from_millis(100));
    println!("主线程：不调用 unpark，等待超时...");
    
    handle.join().unwrap();
    
    println!("关键点：park_timeout 超时后会自动唤醒，不需要 unpark");
    println!();
}

/// 演示主动 drop 锁守卫的原因
fn demo_active_drop() {
    println!("=== 主动 drop 锁守卫的原因 ===");
    
    struct TaskQueue {
        queue: Mutex<VecDeque<u32>>,
        condvar: Condvar,
    }
    
    let task_queue = Arc::new(TaskQueue {
        queue: Mutex::new(VecDeque::new()),
        condvar: Condvar::new(),
    });
    
    let consumer = Arc::clone(&task_queue);
    let producer = Arc::clone(&task_queue);
    
    // 消费者：演示主动 drop 的重要性
    let consumer_handle = thread::spawn(move || {
        loop {
            let mut q = consumer.queue.lock().unwrap();
            
            while q.is_empty() {
                q = consumer.condvar.wait(q).unwrap();
            }
            
            // 取出任务
            let item = q.pop_front().unwrap();
            
            // ✅ 主动 drop：立即释放锁，让生产者能继续添加任务
            drop(q);
            
            // 处理任务（耗时操作，不需要持有锁）
            println!("消费者处理任务: {}（锁已释放）", item);
            thread::sleep(Duration::from_millis(200)); // 模拟耗时处理
            
            if item == 4 {
                break;
            }
        }
    });
    
    // 生产者：快速添加任务
    let producer_handle = thread::spawn(move || {
        for i in 0..5 {
            thread::sleep(Duration::from_millis(100));
            
            let mut q = producer.queue.lock().unwrap();
            q.push_back(i);
            drop(q);
            
            producer.condvar.notify_one();
            println!("生产者添加任务: {}", i);
        }
    });
    
    producer_handle.join().unwrap();
    consumer_handle.join().unwrap();
    
    println!("说明：主动 drop 让锁尽快释放，提高并发效率");
    println!("如果不 drop，锁会持有到整个循环结束，影响其他线程");
    println!();
}

/// 演示 notify_one 和 notify_all 的区别
fn demo_notify_methods() {
    println!("=== notify_one vs notify_all ===");
    
    struct TaskQueue {
        queue: Mutex<VecDeque<u32>>,
        condvar: Condvar,
    }
    
    let task_queue = Arc::new(TaskQueue {
        queue: Mutex::new(VecDeque::new()),
        condvar: Condvar::new(),
    });
    
    let mut handles = vec![];
    
    // 创建 3 个消费者线程
    for i in 0..3 {
        let queue = Arc::clone(&task_queue);
        let handle = thread::spawn(move || {
            let mut q = queue.queue.lock().unwrap();
            while q.is_empty() {
                q = queue.condvar.wait(q).unwrap();
            }
            if let Some(item) = q.pop_front() {
                println!("消费者 {} 处理任务: {}", i, item);
            }
        });
        handles.push(handle);
    }
    
    thread::sleep(Duration::from_millis(100));
    
    // 添加一个任务
    {
        let mut q = task_queue.queue.lock().unwrap();
        q.push_back(100);
        drop(q);
    }
    
    // notify_one：只唤醒一个等待的线程
    task_queue.condvar.notify_one();
    println!("使用 notify_one：只唤醒一个消费者");
    
    thread::sleep(Duration::from_millis(100));
    
    // notify_all：唤醒所有等待的线程
    {
        let mut q = task_queue.queue.lock().unwrap();
        q.push_back(200);
        q.push_back(201);
        drop(q);
    }
    task_queue.condvar.notify_all();
    println!("使用 notify_all：唤醒所有消费者");
    
    for handle in handles {
        handle.join().unwrap();
    }
    
    println!();
}

/// 演示 wait_timeout 的返回值处理
fn demo_wait_timeout_return() {
    println!("=== wait_timeout 的返回值处理 ===");
    
    struct TaskQueue {
        queue: Mutex<VecDeque<u32>>,
        condvar: Condvar,
    }
    
    let task_queue = Arc::new(TaskQueue {
        queue: Mutex::new(VecDeque::new()),
        condvar: Condvar::new(),
    });
    
    let consumer = Arc::clone(&task_queue);
    let producer = Arc::clone(&task_queue);
    
    let consumer_handle = thread::spawn(move || {
        let mut q = consumer.queue.lock().unwrap();
        
        loop {
            // wait_timeout 返回 (MutexGuard, WaitTimeoutResult)
            let timeout = Duration::from_millis(500);
            let result = consumer.condvar.wait_timeout(q, timeout).unwrap();
            
            q = result.0; // 重新获取的锁守卫
            let timed_out = result.1.timed_out(); // 是否超时
            
            if timed_out {
                println!("等待超时（500ms），继续检查条件");
                // 超时后可以继续等待，或者做其他处理
                if q.is_empty() {
                    continue; // 继续等待
                }
            }
            
            // 条件满足，处理任务
            if let Some(item) = q.pop_front() {
                println!("处理任务: {}", item);
                drop(q);
                break;
            }
        }
    });
    
    // 延迟后添加任务
    thread::sleep(Duration::from_millis(800));
    
    {
        let mut q = producer.queue.lock().unwrap();
        q.push_back(42);
        drop(q);
    }
    producer.condvar.notify_one();
    
    consumer_handle.join().unwrap();
    
    println!("说明：wait_timeout 返回超时标志，可以根据情况处理");
    println!();
}

/// 总结 Condvar 和 park 的关键点
fn demo_summary() {
    println!("=== Condvar 和 park 总结 ===");
    println!();
    println!("Condvar（条件变量）:");
    println!("  - 解决同步问题：协调线程执行顺序");
    println!("  - 必须配合 Mutex 使用");
    println!("  - wait() 会自动释放和重新获取锁");
    println!("  - wait_timeout() 带超时，超时后自动唤醒");
    println!("  - notify_one() 唤醒一个等待的线程");
    println!("  - notify_all() 唤醒所有等待的线程");
    println!();
    println!("park/unpark:");
    println!("  - 不依赖锁，简单的线程暂停/恢复");
    println!("  - park() 让线程休眠，等待 unpark()");
    println!("  - park_timeout() 带超时，超时后自动唤醒");
    println!("  - unpark() 唤醒被 park 的线程");
    println!();
    println!("关键区别:");
    println!("  - Condvar 用于等待特定条件（如队列非空）");
    println!("  - park 用于简单的线程暂停/恢复");
    println!("  - 两者都有超时版本，超时后自动唤醒");
    println!("  - 主动 drop 锁守卫可以提前释放锁，提高并发效率");
    println!();
}

pub fn main() {
    demo_condvar_basic();
    demo_wait_vs_park();
    demo_wait_timeout();
    demo_park_timeout();
    demo_active_drop();
    demo_notify_methods();
    demo_wait_timeout_return();
    demo_summary();
}

