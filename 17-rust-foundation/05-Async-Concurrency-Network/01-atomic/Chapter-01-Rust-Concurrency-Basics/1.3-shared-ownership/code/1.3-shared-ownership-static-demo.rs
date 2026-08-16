use std::thread;

static GREETING: &str = "Greetings from a 'static string!";

pub fn main() {
    // &'static str 可以安全地被线程持有。
    let handle = thread::spawn(|| {
        println!("{GREETING}");
    });

    handle.join().unwrap();
}

