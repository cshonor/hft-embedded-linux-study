//! 5.3 方法语法：三种 `self` + 关联函数 + 多 impl 块

#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

// 只读方法
impl Rectangle {
    fn area(&self) -> u32 {
        self.width * self.height
    }

    fn can_hold(&self, other: &Rectangle) -> bool {
        self.width > other.width && self.height > other.height
    }

    fn width(&self) -> bool {
        self.width > 0
    }
}

// 修改方法
impl Rectangle {
    fn set_size(&mut self, w: u32, h: u32) {
        self.width = w;
        self.height = h;
    }

    fn destroy_to_tuple(self) -> (u32, u32) {
        (self.width, self.height)
    }
}

// 关联函数
impl Rectangle {
    fn new(w: u32, h: u32) -> Self {
        Self { width: w, height: h }
    }

    fn square(size: u32) -> Self {
        Self {
            width: size,
            height: size,
        }
    }
}

fn main() {
    println!("=== 三种 self + 关联函数（核心） ===");
    let mut rect = Rectangle::new(10, 20);

    // &self：r.area() → (&r).area()
    let s = rect.area();
    println!("面积：{s}");

    // &mut self：→ (&mut rect).set_size(...)
    rect.set_size(50, 60);
    println!("修改后 {rect:?}");

    // self：move
    let (w, h) = rect.destroy_to_tuple();
    println!("拆分数据：{w} {h}");
    // println!("{:?}", rect); // ❌ moved

    println!("\n=== can_hold：other 须手动 & ===");
    let r1 = Rectangle::new(50, 60);
    let r2 = Rectangle::new(20, 30);
    println!("r1.can_hold(&r2) = {}", r1.can_hold(&r2));

    println!("\n=== 5.2 演进：独立函数 → 方法 ===");
    demo_area_evolution();

    println!("\n=== 关联函数 ::square ===");
    let sq = Rectangle::square(3);
    if sq.width() {
        println!("square = {sq:?}, area = {}", sq.area());
    }
    dbg!(&sq);
}

fn demo_area_evolution() {
    let w = 30;
    let h = 50;
    println!("v1 变量: {}", area_v1(w, h));
    println!("v2 元组: {}", area_v2((w, h)));
    let r = Rectangle::new(w, h);
    println!("v3 &结构体: {}", area_v3(&r));
    println!("v4 方法: {}", r.area());
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
