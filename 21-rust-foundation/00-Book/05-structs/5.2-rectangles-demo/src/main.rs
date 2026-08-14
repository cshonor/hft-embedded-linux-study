// 5.2 使用结构体的代码例子：计算长方形面积

#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

fn main() {
    println!("=== 版本 1：分别用变量 ===");
    let width1 = 30;
    let height1 = 50;
    println!(
        "The area of the rectangle is {} square pixels.",
        area_v1(width1, height1)
    );

    println!("\n=== 版本 2：用元组 ===");
    let rect2 = (30, 50);
    println!(
        "The area of the rectangle is {} square pixels.",
        area_v2(rect2)
    );

    println!("\n=== 版本 3：用结构体 + 借用 ===");
    let rect3 = Rectangle {
        width: 30,
        height: 50,
    };
    println!(
        "The area of the rectangle is {} square pixels.",
        area_v3(&rect3)
    );

    println!("\n=== Debug 打印 ===");
    println!("rect3 is {:?}", rect3);
    println!("rect3 is {:#?}", rect3);

    println!("\n=== dbg! 宏（输出到 stderr） ===");
    let scale = 2;
    let rect4 = Rectangle {
        width: dbg!(30 * scale),
        height: 50,
    };
    dbg!(&rect4);
}

fn area_v1(width: u32, height: u32) -> u32 {
    width * height
}

fn area_v2(dimensions: (u32, u32)) -> u32 {
    dimensions.0 * dimensions.1
}

fn area_v3(rectangle: &Rectangle) -> u32 {
    rectangle.width * rectangle.height
}

