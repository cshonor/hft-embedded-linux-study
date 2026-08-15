// 3.3 函数 - 示例

fn main() {
    println!("=== 1. 函数调用 ===");
    another_function(); // 输出: Another function.

    println!("\n=== 2. 带参数的函数 ===");
    another_function_with_param(5); // 输出: The value of x is: 5
    print_labeled_measurement(5, 'h'); // 输出: The measurement is: 5h

    println!("\n=== 3. 代码块作为表达式 ===");
    let y = {
        let x = 3;
        x + 1 // 无分号，作为块的值
    };
    println!("The value of y is: {}", y); // 输出: 4

    println!("\n=== 4. 带返回值的函数 ===");
    println!("add(1, 2) = {}", add(1, 2));
    println!("five() = {}", five());
    println!("plus_one(5) = {}", plus_one(5));
    println!("max(3, 7) = {}", max(3, 7));
}

fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn max(a: i32, b: i32) -> i32 {
    if a > b {
        return a;
    }
    b
}

fn another_function() {
    println!("Another function.");
}

fn another_function_with_param(x: i32) {
    println!("The value of x is: {}", x);
}

fn print_labeled_measurement(value: i32, unit_label: char) {
    println!("The measurement is: {}{}", value, unit_label);
}

/// 无参数，返回 i32，函数体只有表达式 5
fn five() -> i32 {
    5 // 无分号
}

/// 参数加 1 后返回，注意 x + 1 末尾不能加分号
fn plus_one(x: i32) -> i32 {
    x + 1 // 无分号，作为返回值
}

