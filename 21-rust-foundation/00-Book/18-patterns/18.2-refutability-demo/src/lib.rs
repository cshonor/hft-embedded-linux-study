//! 18.2 可反驳 / 不可反驳模式 demo

/// §三 完整整合 demo（用户笔记 §完整整合 Demo）
pub fn demo_all_pattern_sites() {
    // 1. let（不可反驳）
    let (a, b) = (100, 200);
    println!("  let 解构：{a} {b}");

    // 2. 函数参数模式
    let pos = (5, 8);
    print_coord(&pos);

    // 3. match（可反驳 + 穷尽）
    let opt = Some(666);
    match opt {
        Some(n) => println!("  match 匹配：{n}"),
        None => println!("  match 空值"),
    }

    // 4. if let
    if let Some(val) = opt {
        println!("  if let 匹配：{val}");
    }

    // 5. while let
    let mut stack = vec!["rust", "pattern", "demo"];
    println!("  while let 出栈：");
    while let Some(s) = stack.pop() {
        println!("    {s}");
    }

    // 6. for 解构（不可反驳模式）
    println!("  for 循环解构：");
    let nums = vec![10, 20, 30];
    for (i, num) in nums.iter().enumerate() {
        println!("    下标{i}：{num}");
    }
}

pub fn print_coord(&(x, y): &(i32, i32)) {
    println!("  坐标：({x}, {y})");
}

pub fn demo_refutable_vs_irrefutable_notes() {
    let some_option_value = Some(3u8);

    if let Some(x) = some_option_value {
        println!("  if let Some(x) => {x}（可反驳，OK）");
    }

    // let Some(x) = some_option_value;  // E0005

    let (x, _) = (1, 2);
    println!("  let (x, _) = (1,2) → x={x}（不可反驳，OK）");

    println!("  见源码注释：let Some / fn Some(param) / for Some(x) 均不能编译");
}

#[allow(irrefutable_let_patterns)]
pub fn demo_irrefutable_if_let_warning() {
    if let x = 5 {
        println!("  if let x = 5 => {x}（应改用 let；此处演示警告）");
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tuple_let_destructure() {
        let (a, b) = (1, 2);
        assert_eq!(a + b, 3);
    }

    #[test]
    fn if_let_some() {
        let opt = Some(42);
        let mut got = 0;
        if let Some(n) = opt {
            got = n;
        }
        assert_eq!(got, 42);
    }
}

// 编译失败示例（文档用）：
// fn bad_fn(Some(val): Option<i32>) {}
// fn main() { let Some(x) = None::<i32>; }
