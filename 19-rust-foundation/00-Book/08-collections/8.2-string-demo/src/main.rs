// 8.2 String / &str（UTF-8）- 示例

fn main() {
    println!("=== 1) 新建 String ===");
    let mut s = String::new();
    s.push_str("initial");
    println!("s = {}", s);

    let s = "initial contents".to_string();
    let s_from = String::from("initial contents");
    println!("to_string = {}, from = {}", s, s_from);

    println!("\n=== 2) UTF-8 文本 ===");
    let hello = String::from("你好");
    let hola = String::from("Hola");
    let rus = String::from("Здравствуйте");
    println!("{} / {} / {}", hello, hola, rus);
    println!("len(你好)={} bytes, len(Hola)={} bytes, len(Здравствуйте)={} bytes", hello.len(), hola.len(), rus.len());

    println!("\n=== 3) push_str / push ===");
    let mut s = String::from("foo");
    let s2 = "bar";
    s.push_str(s2); // &str，不拿走 s2
    println!("s = {}, s2 = {}", s, s2);

    let mut s = String::from("lo");
    s.push('l');
    println!("push char -> {}", s);

    println!("\n=== 4) + 拼接 vs format! ===");
    let s1 = String::from("Hello, ");
    let s2 = String::from("world!");
    let s3 = s1 + &s2; // s1 moved
    println!("s3 = {}, s2 still ok = {}", s3, s2);

    let a = String::from("tic");
    let b = String::from("tac");
    let c = String::from("toe");
    let s = format!("{}-{}-{}", a, b, c);
    println!("format -> {}", s);

    println!("\n=== 5) 字符串 slice（range） ===");
    let hello = "Здравствуйте";
    let s = &hello[0..4]; // "Зд"
    println!("slice = {}", s);

    // 下面会在运行时 panic（字节边界不是字符边界），保留为注释：
    // let bad = &hello[0..1];

    println!("\n=== 6) 遍历 chars() / bytes() ===");
    for c in "नमस्ते".chars() {
        print!("{c} ");
    }
    println!();
    for b in "नमस्ते".bytes().take(6) {
        print!("{b} ");
    }
    println!("... (bytes truncated)");

    println!("\n=== 7) String 不能索引 ===");
    println!("Rust 不允许 s[0]，因为 UTF-8 下“第 0 个字符”不明确且无法保证 O(1)。");

    println!("\n=== 8) 底层结构：size_of + 胖指针 ===");
    use std::mem::size_of;
    println!(
        "String={}B  &str={}B  [i32;3]={}B  &[i32]={}B  Vec<i32>={}B",
        size_of::<String>(),
        size_of::<&str>(),
        size_of::<[i32; 3]>(),
        size_of::<&[i32]>(),
        size_of::<Vec<i32>>(),
    );

    let literal: &str = "abc";
    let owned = String::from("abc");
    let borrowed: &str = &owned;
    println!("literal ptr={:p} len={}", literal.as_ptr(), literal.len());
    println!("owned   ptr={:p} len={} cap={}", owned.as_ptr(), owned.len(), owned.capacity());
    println!("borrowed ptr={:p} (借 String 堆，与 owned 同址)", borrowed.as_ptr());

    let arr = [10, 20, 30, 40];
    let slice: &[i32] = &arr[1..3];
    println!("arr[1..3] ptr={:p} len={}", slice.as_ptr(), slice.len());
    println!("\nok: 见 8.2.5 四种类型底层结构");
}

