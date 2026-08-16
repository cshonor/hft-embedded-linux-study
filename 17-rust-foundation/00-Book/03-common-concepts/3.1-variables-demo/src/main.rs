// 3.1 变量和可变性 - 示例
// 运行 cargo run 输出如下：

fn main() {
    println!("=== 1. 可变变量 mut ===");
    let mut x = 5;
    println!("The value of x is: {}", x); // 输出: The value of x is: 5
    x = 6;
    println!("The value of x is: {}", x); // 输出: The value of x is: 6

    println!("\n=== 2. 常量 const ===");
    const THREE_HOURS_IN_SECONDS: u32 = 60 * 60 * 3;
    println!("THREE_HOURS_IN_SECONDS = {}", THREE_HOURS_IN_SECONDS); // 输出: 10800

    println!("\n=== 3. 遮蔽 shadowing ===");
    let x = 5;
    let x = x + 1; // 遮蔽，x 变为 6
    {
        let x = x * 2; // 内部作用域遮蔽，x 变为 12
        println!("The value of x in the inner scope is: {}", x); // 输出: 12
    }
    println!("The value of x is: {}", x); // 输出: 6

    println!("\n=== 4. 遮蔽改变类型 ===");
    let spaces = "   ";
    let spaces = spaces.len(); // 从 &str 变为 usize
    println!("spaces = {} (类型变为 usize)", spaces); // 输出: 3

    println!("\n=== 5. Default 默认值（须显式调用）===");
    let num: i32 = Default::default();
    let flag: bool = Default::default();
    let s: String = Default::default();
    let v: Vec<i32> = Vec::default();
    println!("num={}, flag={}, s=[{}], v={:?}", num, flag, s, v);
    let s2: String = String::default();
    println!("String::default() 同 Default::default(): [{}]", s2);

    // 多类型未初始化均报错（取消注释会编译失败）：
    // let n: i32;
    // let f: bool;
    // println!("{} {}", n, f);

    println!("\n=== 6. main 内：先声明后赋值（须 mut）===");
    let flag = std::hint::black_box(true);
    let mut a: i32;
    if flag {
        a = 10;
    } else {
        a = 0;
    }
    println!("先 let mut 再按分支赋值: a = {}", a);
}

