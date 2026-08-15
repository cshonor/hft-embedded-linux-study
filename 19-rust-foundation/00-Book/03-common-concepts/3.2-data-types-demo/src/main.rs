// 3.2 数据类型 - 示例

fn main() {
    println!("=== 1. 类型标注 ===");
    let guess: u32 = "42".parse().expect("Not a number!");
    println!("guess = {}", guess); // 输出: 42

    println!("\n=== 2. 整型字面量 ===");
    let decimal = 98_222;
    let same_3 = 98_222;
    let same_4 = 9_8222;
    let grouped = 1234_5678;
    let hex = 0xFF_AA_00;
    let binary = 0b1010_1100;
    let octal = 0o77;
    let byte = b'A';
    println!("98222 == 98_222 == 9_8222 ? {} {}", decimal == same_3, same_3 == same_4);
    println!("1234_5678 = {}", grouped);
    println!("hex=0x{:X}, bin={}, octal={}, byte={}", hex, binary, octal, byte);

    println!("\n=== 3. 浮点类型 ===");
    let x = 2.0; // f64
    let y: f32 = 3.0;
    println!("x(f64)={}, y(f32)={}", x, y); // 输出: 2, 3

    println!("\n=== 4. 数字运算 ===");
    let sum = 5 + 10;           // 15
    let difference = 95.5 - 4.3; // 91.2
    let product = 4 * 30;       // 120
    let quotient = 56.7 / 32.2;
    let floored = 2 / 3;        // 0 整数除法向下取整
    let remainder = 43 % 5;     // 3
    println!("sum={}, diff={}, product={}, quotient={:.2}, floored={}, remainder={}",
        sum, difference, product, quotient, floored, remainder);

    println!("\n=== 5. 布尔类型 ===");
    let t = true;
    let f: bool = false;
    println!("t={}, f={}", t, f); // 输出: true, false

    println!("\n=== 6. 字符类型 char ===");
    let c = 'z';
    let z = 'ℤ';
    let heart_eyed_cat = '😻';
    let zhong = '中';
    println!("c={}, z={}, emoji={}, zhong={}", c, z, heart_eyed_cat, zhong);

    println!("\n=== 7. 元组 tuple ===");
    let tup: (i32, f64, u8) = (500, 6.4, 1);
    let (x, y, z) = tup;
    println!("解构: x={}, y={}, z={}", x, y, z);
    println!("索引: tup.0={}, tup.1={}, tup.2={}", tup.0, tup.1, tup.2);

    // 函数返回多个值时常用元组
    let (sum, product) = add_and_mul(10, 3);
    println!("add_and_mul(10,3) -> sum={}, product={}", sum, product);

    println!("\n=== 8. 数组 array ===");
    let a = [1, 2, 3, 4, 5];
    let b: [i32; 5] = [1, 2, 3, 4, 5];
    let c = [3; 5];  // [3, 3, 3, 3, 3]
    println!("a[0]={}, a[1]={}", a[0], a[1]); // 1, 2
    println!("c = {:?}", c);  // [3, 3, 3, 3, 3]

    let months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    println!("months[0]={}", months[0]); // Jan
}

fn add_and_mul(a: i32, b: i32) -> (i32, i32) {
    (a + b, a * b)
}

