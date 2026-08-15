// 4.1 什么是所有权 - 示例

const MAX: i32 = 100;
const TXT: &str = "rust";

static NUM: i32 = 100;
static MSG: &str = "全局静态字符串";
static GLOBAL_STR: &str = "全局字面量";
static mut GLOBAL_MUT: i32 = 0;

fn main() {
    println!("=== 0. 栈 vs 堆 ===");
    // 栈：固定大小，赋值直接拷贝
    let num: i32 = 42;
    let num2 = num;
    println!("栈上 i32 拷贝: num = {}, num2 = {}", num, num2);

    // 堆：String 的 ptr/len/cap 在栈，字节在堆
    let s = String::from("hello");
    println!(
        "String 栈上 ptr/len/cap → 堆上 {} 字节: \"{}\"",
        s.len(),
        s
    );

    println!("\n=== 0.5 所有者、作用域、static、字面量 ===");
    {
        let local = String::from("局部变量");
        println!("局部变量（块内有效）: {}", local);
    } // local 出作用域，堆内存释放

    // static 只能模块顶层；函数内禁止 static INNER: i32 = 200;
    println!("static NUM = {}, static MSG = {}", NUM, MSG);

    let local_str = "函数内字面量"; // 只有局部变量 local_str；"xxx" 是字面数据，不是变量
    println!("GLOBAL_STR = {}, local_str = {}", GLOBAL_STR, local_str);

    // const：编译期内联，无所有权，出块只是不能访问
    const COUNT: i32 = 100;
    {
        const NUM: i32 = 200;
        println!("const COUNT = {}, NUM = {}", COUNT, NUM);
    }
    println!("出块后 COUNT 仍可用 = {}", COUNT);
    // println!("{}", NUM); // ❌ 超出作用域，不是释放内存

    let literal = "hello"; // s 是局部引用；指向的字面量本体 'static
    let owned = String::from("hello"); // 堆上，变量拥有所有权
    println!("字面量引用: {}, 堆上 String: {}", literal, owned);

    println!("\n=== 0.6 const vs static ===");
    const LOCAL_MSG: &str = "函数内 const";
    println!("const MAX = {}, LOCAL_MSG = {}", MAX, LOCAL_MSG);

    let static_ref: &'static i32 = &NUM; // static 有全局地址，引用为 'static
    println!("static NUM = {}, &'static ref = {}", NUM, static_ref);

    unsafe {
        GLOBAL_MUT = 10;
        let v = GLOBAL_MUT; // 拷贝值，避免直接引用 static mut
        println!("static mut GLOBAL_MUT = {}", v);
    }
    // let a = 10; const B: i32 = a;  // ❌ 不能用运行时变量初始化

    println!("\n=== 0.7 const vs 字符串字面量 ===");
    let literal = "hello";
    println!("字面量 \"hello\" 地址: {:p}", literal);

    let a = MAX; // 编译后等价于 let a = 100;
    println!("const MAX 内联: a = {}", a);

    let s = TXT; // 编译后等价于 let s = &"rust";
    println!("const TXT → 指向只读段: \"{}\" 地址: {:p}", s, s);

    println!("\n=== 0.8 字面量引用：变量消失，数据不 drop ===");
    {
        let s = "块内字面量"; // 栈上引用变量
        let owned = String::from("块内 String");
        println!("块内: s = {}, owned = {}", s, owned);
    } // s 消失，"块内字面量" 仍在只读段；owned drop 释放堆

    println!("\n=== 1. 移动 (Move) ===");
    let s1 = String::from("hello");
    let s2 = s1; // s1 被移动，s1 不再有效
    println!("s2 = {}", s2); // s2 有效

    println!("\n=== 2. 克隆 (Clone) ===");
    let s1 = String::from("hello");
    let s2 = s1.clone(); // 深拷贝堆数据
    println!("s1 = {}, s2 = {}", s1, s2); // 两者都有效

    println!("\n=== 3. Copy 类型 ===");
    let x = 5;
    let y = x; // 拷贝，不是移动
    println!("x = {}, y = {}", x, y); // 两者都有效

    println!("\n=== 4. 所有权与函数 ===");
    let s = String::from("hello");
    takes_ownership(s); // s 被移动进函数
                        // s 不再有效

    let x = 5;
    makes_copy(x); // x 被拷贝
    println!("main: x 仍有效 = {}", x); // x 仍有效

    println!("\n=== 5. 返回值转移所有权 ===");
    let s1 = gives_ownership();
    let s2 = String::from("hello");
    let s3 = takes_and_gives_back(s2); // s2 被移动，返回值给 s3
    println!("s1 = {}, s3 = {}", s1, s3);

    println!("\n=== 6. 用元组返回所有权 ===");
    let s1 = String::from("hello");
    let (s2, len) = calculate_length(s1);
    println!("The length of '{}' is {}.", s2, len);
}

fn takes_ownership(some_string: String) {
    println!("takes_ownership: {}", some_string);
} // some_string 离开作用域，drop 被调用

fn makes_copy(some_integer: i32) {
    println!("makes_copy: {}", some_integer);
} // some_integer 离开作用域，无特殊操作（栈上）

fn gives_ownership() -> String {
    let some_string = String::from("yours");
    some_string // 返回并移动给调用者
}

fn takes_and_gives_back(a_string: String) -> String {
    a_string // 原样返回，移动给调用者
}

fn calculate_length(s: String) -> (String, usize) {
    let length = s.len();
    (s, length) // 返回 s，把所有权还回去
}

