use std::sync::mpsc;
use std::thread;
use std::time::Duration;

/// 教材为 `Duration::from_secs(1)`；此处略短，便于反复运行。
const STEP: Duration = Duration::from_millis(200);

/// 示例 16-7 / 16-8：单发单收，打印 `Got: hi`
fn example_single_message() {
    println!("\n=== 16-8: 单条消息 ===");
    let (tx, rx) = mpsc::channel();

    thread::spawn(move || {
        let val = String::from("hi");
        tx.send(val).unwrap();
    });

    let received = rx.recv().unwrap();
    println!("Got: {received}");
}

/// 示例 16-10：一个生产者连续发送多条，`for received in rx`
fn example_single_producer_many_messages() {
    println!("\n=== 16-10: 单生产者，多条（间隔发送）===");
    let (tx, rx) = mpsc::channel();

    thread::spawn(move || {
        let vals = vec![
            String::from("hi"),
            String::from("from"),
            String::from("the"),
            String::from("thread"),
        ];

        for val in vals {
            tx.send(val).unwrap();
            thread::sleep(STEP);
        }
    });

    for received in rx {
        println!("Got: {received}");
    }
}

/// 示例 16-11：克隆 `tx`，两个生产者向同一 `rx` 发送
fn example_multiple_producers() {
    println!("\n=== 16-11: 多生产者（顺序因调度而异）===");
    let (tx, rx) = mpsc::channel();

    let tx1 = tx.clone();
    thread::spawn(move || {
        let vals = vec![
            String::from("hi"),
            String::from("from"),
            String::from("the"),
            String::from("thread"),
        ];

        for val in vals {
            tx1.send(val).unwrap();
            thread::sleep(STEP);
        }
    });

    thread::spawn(move || {
        let vals = vec![
            String::from("more"),
            String::from("messages"),
            String::from("for"),
            String::from("you"),
        ];

        for val in vals {
            tx.send(val).unwrap();
            thread::sleep(STEP);
        }
    });

    for received in rx {
        println!("Got: {received}");
    }
}

fn main() {
    example_single_message();
    example_single_producer_many_messages();
    example_multiple_producers();
}
