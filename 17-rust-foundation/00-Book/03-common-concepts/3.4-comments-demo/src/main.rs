// 3.4 注释 - 示例

// 这是单行注释，编译器会忽略

/*
 * 块注释也可以这样写 (/* ... */)
 * 但 Rust 惯用每行 // 的形式
 */

fn main() {
    // 推荐：注释放到代码上方单独一行
    let lucky_number = 7;
    println!("lucky_number = {}", lucky_number); // 输出: 7

    // 多行注释示例：
    // 当需要较长的说明时，
    // 可以每行都以 // 开头
    let x = 42;
    println!("x = {}", x);

    println!("add(1, 2) = {}", add(1, 2)); // 输出: 3
}

/// 文档注释：用于生成 API 文档
/// 计算两数之和
fn add(a: i32, b: i32) -> i32 {
    a + b
}

