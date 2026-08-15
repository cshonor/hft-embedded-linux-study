use std::thread;

pub fn main() {
    // 把字符串放到堆上并泄漏，得到 &'static str。
    let leaked: &'static str = {
        let message = String::from("Hello from a leaked Box!");
        Box::leak(message.into_boxed_str())
    };

    // 现在线程可以持有这个 'static 引用。
    let handle = thread::spawn(move || {
        println!("{leaked}");
    });

    handle.join().unwrap();
}

