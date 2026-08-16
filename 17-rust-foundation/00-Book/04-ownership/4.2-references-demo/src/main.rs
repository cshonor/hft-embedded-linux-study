// 4.2 引用与借用 - 示例

fn main() {
    println!("=== 1. 不可变引用（借用）===");
    let s1 = String::from("hello");
    let len = calculate_length(&s1);
    println!("The length of '{}' is {}.", s1, len); // s1 仍有效

    println!("\n=== 2. 可变引用 ===");
    let mut s = String::from("hello");
    change(&mut s);
    println!("{}", s); // hello, world

    println!("\n=== 3. 作用域错开：多个可变引用 ===");
    let mut s = String::from("hello");
    {
        let r1 = &mut s;
        r1.push_str(" (r1)");
    } // r1 离开作用域
    let r2 = &mut s;
    r2.push_str(" (r2)");
    println!("{}", s);

    println!("\n=== 4. NLL：不可变引用用完后可变引用 ===");
    let mut s = String::from("hello");
    let r1 = &s;
    let r2 = &s;
    println!("{} and {}", r1, r2); // r1、r2 最后一次使用
    let r3 = &mut s; // ✓ r1、r2 作用域已结束
    r3.push_str(", world");
    println!("{}", r3);

    println!("\n=== 5. 正确返回所有权，避免悬垂 ===");
    let s = no_dangle();
    println!("{}", s);
    // 生命周期 'a / 'static 详见 10.3-lifetimes-demo
}

/// 借用：不获取所有权，s1 在调用后仍有效
fn calculate_length(s: &String) -> usize {
    s.len()
}

/// 可变引用：可修改借用的值
fn change(some_string: &mut String) {
    some_string.push_str(", world");
}

/// 返回 String 所有权，而非引用，避免悬垂引用
fn no_dangle() -> String {
    let s = String::from("hello");
    s // 转移所有权
}

