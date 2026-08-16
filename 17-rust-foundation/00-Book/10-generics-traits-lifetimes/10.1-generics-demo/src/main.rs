// 10.1 泛型数据类型 - 示例

use std::fmt::Debug;

// 通过 trait bound 限制 T：必须可比较，并且可 Copy（这样返回 T 不需要 move）
fn largest<T: PartialOrd + Copy>(list: &[T]) -> T {
    let mut largest = list[0];
    for &item in list.iter() {
        if item > largest {
            largest = item;
        }
    }
    largest
}

#[derive(Debug)]
struct PointSame<T> {
    x: T,
    y: T,
}

#[derive(Debug)]
struct Point<T, U> {
    x: T,
    y: U,
}

impl<T> PointSame<T> {
    fn x(&self) -> &T {
        &self.x
    }
}

// 只为 Point<f32, f32> 提供方法
impl Point<f32, f32> {
    fn distance_from_origin(&self) -> f32 {
        (self.x.powi(2) + self.y.powi(2)).sqrt()
    }
}

impl<T, U> Point<T, U> {
    fn mixup<V, W>(self, other: Point<V, W>) -> Point<T, W> {
        Point {
            x: self.x,
            y: other.y,
        }
    }
}

fn print_debug<T: Debug>(x: T) {
    println!("{:?}", x);
}

fn main() {
    println!("=== 1) 泛型函数 largest ===");
    let number_list = vec![34, 50, 25, 100, 65];
    let result = largest(&number_list);
    println!("largest number = {}", result);

    let char_list = vec!['y', 'm', 'a', 'q'];
    let result = largest(&char_list);
    println!("largest char = {}", result);

    println!("\n=== 2) 泛型结构体 PointSame<T> ===");
    let integer = PointSame { x: 5, y: 10 };
    let float = PointSame { x: 1.0, y: 4.0 };
    print_debug(integer);
    print_debug(float);

    println!("\n=== 3) 多泛型结构体 Point<T, U> ===");
    let p1 = Point { x: 5, y: 10.4 };
    let p2 = Point { x: "Hello", y: 'c' };
    let p3 = p1.mixup(p2);
    println!("p3.x = {}, p3.y = {}", p3.x, p3.y);

    println!("\n=== 4) 只对具体类型实现方法 ===");
    let p = Point { x: 3.0f32, y: 4.0f32 };
    println!("distance_from_origin = {}", p.distance_from_origin());

    println!("\n=== 5) 方法返回引用 ===");
    let p = PointSame { x: "abc", y: "def" };
    println!("p.x() = {}", p.x());

    println!("\n=== 6) Option/Result 也是泛型枚举 ===");
    let integer = Some(5);
    let float = Some(5.0);
    println!("integer = {:?}, float = {:?}", integer, float);

    let ok: Result<i32, &str> = Ok(123);
    let err: Result<i32, &str> = Err("oops");
    println!("ok = {:?}, err = {:?}", ok, err);
}

