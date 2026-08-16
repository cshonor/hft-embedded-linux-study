//! 示例 18-1～18-7：模式出现的位置（match / if let 链 / while let / for / let / 函数参数）。

fn example_18_1_if_let_chain() {
    let favorite_color: Option<&str> = None;
    let is_tuesday = false;
    let age: Result<u8, _> = "34".parse();

    if let Some(color) = favorite_color {
        println!("Using your favorite color, {color}, as the background");
    } else if is_tuesday {
        println!("Tuesday is green day!");
    } else if let Ok(age) = age {
        if age > 30 {
            println!("Using purple as the background color");
        } else {
            println!("Using orange as the background color");
        }
    } else {
        println!("Using blue as the background color");
    }
}

fn example_18_2_while_let_stack() {
    let mut stack = Vec::new();
    stack.push(1);
    stack.push(2);
    stack.push(3);

    while let Some(top) = stack.pop() {
        println!("{top}");
    }
}

fn example_18_3_for_destructure() {
    let v = vec!['a', 'b', 'c'];
    for (index, value) in v.iter().enumerate() {
        println!("{value} is at index {index}");
    }
}

fn example_18_4_let_tuple() {
    let (x, y, z) = (1, 2, 3);
    println!("let tuple: x={x}, y={y}, z={z}");
}

fn print_coordinates(&(x, y): &(i32, i32)) {
    println!("Current location: ({x}, {y})");
}

fn main() {
    println!("--- 18-1 if let chain ---");
    example_18_1_if_let_chain();

    println!("--- 18-2 while let ---");
    example_18_2_while_let_stack();

    println!("--- 18-3 for (index, value) ---");
    example_18_3_for_destructure();

    println!("--- 18-4 let (x,y,z) ---");
    example_18_4_let_tuple();

    println!("--- 18-7 fn arg pattern ---");
    let point = (3, 5);
    print_coordinates(&point);
}
