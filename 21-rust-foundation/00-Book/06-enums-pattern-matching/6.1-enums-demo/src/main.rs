// 6.1 定义枚举 — 完整 demo

#[derive(Debug, Clone, Copy)]
enum IpAddrKind {
    V4,
    V6,
}

#[derive(Debug)]
struct IpAddrLegacy {
    kind: IpAddrKind,
    address: String,
}

#[derive(Debug)]
enum IpAddr {
    V4(u8, u8, u8, u8),
    V6(String),
    Unknown,
}

#[derive(Debug)]
struct User {
    name: String,
}

#[derive(Debug)]
struct Point {
    x: i32,
    y: i32,
}

#[derive(Debug)]
enum Test {
    Item1(u8, i32, String),
    Item2(User, Point),
}

enum Message {
    Quit,
    Move { x: i32, y: i32 },
    Write(String),
    ChangeColor(u8, u8, u8),
}

impl Message {
    fn call(&self) {
        println!("Message::call");
    }
}

fn handle_message(msg: &Message) {
    match msg {
        Message::Quit => println!("  [match] 退出"),
        Message::Move { x, y } => println!("  [match] 移动坐标：{} {}", x, y),
        Message::Write(s) => println!("  [match] 消息内容：{}", s),
        Message::ChangeColor(r, g, b) => println!("  [match] 颜色：{} {} {}", r, g, b),
    }
}

fn route(ip_type: IpAddrKind) {
    println!("routing {:?}", ip_type);
}

enum MyOption<T> {
    Some(T),
    None,
}

impl<T> MyOption<T> {
    fn unwrap(self) -> T {
        match self {
            MyOption::Some(v) => v,
            MyOption::None => panic!("unwrap on MyOption::None"),
        }
    }

    fn unwrap_or(self, default: T) -> T {
        match self {
            MyOption::Some(v) => v,
            MyOption::None => default,
        }
    }
}

fn get_v4(ip: IpAddr) -> (u8, u8, u8, u8) {
    match ip {
        IpAddr::V4(a, b, c, d) => (a, b, c, d),
        _ => panic!("不是 ipv4"),
    }
}

fn main() {
    println!("=== 1) 单元变体 ===");
    let four = IpAddrKind::V4;
    route(four);

    println!("\n=== 2) 老式 enum + struct（不推荐）===");
    let legacy = IpAddrLegacy {
        kind: IpAddrKind::V4,
        address: String::from("127.0.0.1"),
    };
    println!("legacy = {:?}", legacy);

    println!("\n=== 3) 元组变体 () — 类型永远是 IpAddr ===");
    let a: IpAddr = IpAddr::V4(192, 168, 1, 1);
    let b: IpAddr = IpAddr::V6(String::from("::1"));
    let c: IpAddr = IpAddr::Unknown;
    println!("a={:?} b={:?} c={:?}", a, b, c);

    // 对标 Option：类型是 Option<i32>，不是 Some/None
    let x: Option<i32> = Some(5);
    let y: Option<i32> = None;
    println!("Option: x={:?} y={:?}", x, y);

    // use 导入变体构造器后可省略 IpAddr:: 前缀 → 见 6.1.2
    use IpAddr::{V4, V6};
    let d: IpAddr = V4(10, 0, 0, 1);
    let _e: IpAddr = V6(String::from("fe80::1"));
    println!("use V4/V6 后: d={:?}", d);

    let t1 = Test::Item1(1, 42, String::from("hi"));
    let t2 = Test::Item2(
        User {
            name: String::from("张三"),
        },
        Point { x: 10, y: 20 },
    );
    println!("Item1 {:?}, Item2 {:?}", t1, t2);

    println!("\n=== 4) 标准库 std::net::IpAddr ===");
    use std::net::{IpAddr as StdIpAddr, Ipv4Addr};
    let ip: StdIpAddr = StdIpAddr::V4(Ipv4Addr::new(127, 0, 0, 1));
    println!("is_loopback = {}", ip.is_loopback());

    println!("\n=== 5) Message 四种变体：构造 + match 取值 ===");
    let m1 = Message::Quit;
    let m2 = Message::Move { x: 10, y: 20 };
    let m3 = Message::Write(String::from("你好"));
    let m4 = Message::ChangeColor(255, 0, 0);
    m1.call();
    for m in [&m1, &m2, &m3, &m4] {
        handle_message(m);
    }

    println!("\n=== 6) Option<T> ===");
    let x = Some(10);
    let empty: Option<i32> = None;
    println!(
        "is_some={} is_none={} unwrap_or={}",
        x.is_some(),
        empty.is_none(),
        empty.unwrap_or(0)
    );
    // let sum = 5 + x; // ❌ Option<i32> 与 i32 不能混用

    println!("\n=== 7) 四种取值：match / if let / unwrap / let else ===");
    let opt = Some(10);
    match opt {
        Some(x) => println!("match Some → {}", x),
        None => println!("match None"),
    }

    let ip = IpAddr::V4(192, 168, 1, 2);
    if let IpAddr::V4(w, _, _, _) = ip {
        println!("if let V4 第一段 → {}", w);
    }

    println!("unwrap Some(5) → {}", Some(5).unwrap());
    println!("unwrap_or None → {}", None::<i32>.unwrap_or(0));

    let Some(data) = Some(88) else {
        return;
    };
    println!("let else → {}", data);

    let my = MyOption::Some(42);
    println!("MyOption::unwrap → {}", my.unwrap());
    println!("MyOption::unwrap_or → {}", MyOption::<i32>::None.unwrap_or(0));
    println!("get_v4 → {:?}", get_v4(IpAddr::V4(1, 2, 3, 4)));

    println!("\nok: 标签解构 · MyOption unwrap · 见 6.1.3");
}
