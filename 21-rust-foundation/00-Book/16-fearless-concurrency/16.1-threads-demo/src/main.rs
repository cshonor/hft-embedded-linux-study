use std::thread;
use std::time::Duration;

/// 示例 16-1：未 join 时，主线程先结束则子线程可能未跑完。
fn example_spawn_without_join() {
    println!("\n=== 16-1: spawn（无 join）===");
    thread::spawn(|| {
        for i in 1..10 {
            println!("hi number {i} from the spawned thread!");
            thread::sleep(Duration::from_millis(1));
        }
    });

    for i in 1..5 {
        println!("hi number {i} from the main thread!");
        thread::sleep(Duration::from_millis(1));
    }
}

/// 示例 16-2：`join` 放在主循环之后，子线程会跑完。
fn example_join_after_main_loop() {
    println!("\n=== 16-2: join 在主循环之后 ===");
    let handle = thread::spawn(|| {
        for i in 1..10 {
            println!("hi number {i} from the spawned thread!");
            thread::sleep(Duration::from_millis(1));
        }
    });

    for i in 1..5 {
        println!("hi number {i} from the main thread!");
        thread::sleep(Duration::from_millis(1));
    }

    handle.join().unwrap();
}

/// `join` 若在主循环之前：先执行完子线程，再跑主循环（输出不交错）。
fn example_join_before_main_loop() {
    println!("\n=== join 在主循环之前（顺序执行）===");
    let handle = thread::spawn(|| {
        for i in 1..10 {
            println!("hi number {i} from the spawned thread!");
            thread::sleep(Duration::from_millis(1));
        }
    });

    handle.join().unwrap();

    for i in 1..5 {
        println!("hi number {i} from the main thread!");
        thread::sleep(Duration::from_millis(1));
    }
}

/// 示例 16-5：`move` 将 `v` 移入新线程。
fn example_move_closure() {
    println!("\n=== 16-5: move 闭包 ===");
    let v = vec![1, 2, 3];

    let handle = thread::spawn(move || {
        println!("Here's a vector: {v:?}");
    });

    handle.join().unwrap();
}

fn main() {
    example_spawn_without_join();
    // 给 16-1 子线程一点时间输出（仍不保证跑满 9 次）
    thread::sleep(Duration::from_millis(30));

    example_join_after_main_loop();
    example_join_before_main_loop();
    example_move_closure();
}
