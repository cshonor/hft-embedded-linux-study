//! Rust 与 Unicode：char / UTF-8 字符串 / 实用模板

fn main() {
    println!("=== 1. char ===");
    let ch1 = 'a';
    let ch2 = '中';
    let ch3 = '😀';
    println!("ch1={}, ch2={}, ch3={}", ch1, ch2, ch3);
    println!("'中' as u32 = {}", ch2 as u32);
    if let Some(c) = char::from_u32(20013) {
        println!("from_u32(20013) = {}", c);
    }
    let escaped = '\u{4E2D}';
    println!("\\u{{4E2D}} = {}, is_alphabetic={}", escaped, escaped.is_alphabetic());
    println!("'9'.is_ascii_digit() = {}", '9'.is_ascii_digit());

    println!("\n=== 2. String / &str UTF-8 ===");
    let s = "中文";
    println!("\"中文\".len() 字节数 = {}", s.len());
    print!("bytes: ");
    for b in s.bytes() {
        print!("{b} ");
    }
    println!();
    print!("chars: ");
    for c in s.chars() {
        print!("{c} ");
    }
    println!();

    let s2 = "Rust编程";
    println!("第3个标量: {}", char_at(s2, 2).unwrap());
    let chars: Vec<char> = s2.chars().collect();
    println!("collect[2] = {}", chars[2]);

    println!("\n=== 2b. char 拼进 String（码点 → UTF-8）===");
    let c = '国';
    let mut s = "中".to_string();
    let len_before = s.len();
    s.push(c);
    println!("push('国') 后: \"{}\"", s);
    println!("字节数: {} -> {}（'国' UTF-8 通常 +3 字节）", len_before, s.len());
    println!("'国' 码点 = 0x{:X}", c as u32);

    println!("\n=== 3. 字符数 vs 字节数 ===");
    let s3 = "Hello 世界 😂";
    println!("len() 字节 = {}, chars().count() 标量 = {}", s3.len(), s3.chars().count());

    println!("\n=== 4. 实用模板：截取 / 跳过 / 翻转（按 char）===");
    let text = "Hello 世界 😂";
    println!("take_chars(0,5) = {:?}", take_chars(text, 0, 5));
    println!("skip_chars(6) = {:?}", skip_chars(text, 6));
    println!("reverse_chars = {:?}", reverse_chars(text));

    println!("\n=== 5. 按字节切片（须在字符边界）===");
    let en = "hello";
    println!("&en[0..2] = {:?}", &en[0..2]);
    let zh = "你好";
    // 安全：整段或边界检查
    if let Some(sub) = zh.get(0..3) {
        println!("get(0..3) 第一个汉字 = {:?}", sub);
    }
    println!("is_char_boundary(zh, 3) = {}", is_char_boundary(zh, 3));
}

/// 第 n 个 Unicode 标量（0-based），越界返回 None
fn char_at(s: &str, n: usize) -> Option<char> {
    s.chars().nth(n)
}

/// 取从 start 起共 count 个标量（不是字节）
fn take_chars(s: &str, start: usize, count: usize) -> String {
    s.chars().skip(start).take(count).collect()
}

/// 跳过前 n 个标量，返回剩余部分
fn skip_chars(s: &str, n: usize) -> String {
    s.chars().skip(n).collect()
}

/// 按标量翻转（不是按字节翻转）
fn reverse_chars(s: &str) -> String {
    s.chars().rev().collect()
}

fn is_char_boundary(s: &str, index: usize) -> bool {
    if index == 0 || index == s.len() {
        return true;
    }
    s.is_char_boundary(index)
}
