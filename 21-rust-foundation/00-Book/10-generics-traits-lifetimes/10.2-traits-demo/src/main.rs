// 10.2 trait：定义共享的行为 - 示例

use std::fmt::{Debug, Display};

// --- 1) 定义 trait ---
trait Summary {
    fn summarize_author(&self) -> String;

    fn summarize(&self) -> String {
        format!("(Read more from {}...)", self.summarize_author())
    }
}

#[derive(Debug)]
struct NewsArticle {
    headline: String,
    location: String,
    author: String,
    content: String,
}

#[derive(Debug)]
struct Tweet {
    username: String,
    content: String,
    reply: bool,
    retweet: bool,
}

// --- 2) 为类型实现 trait ---
impl Summary for NewsArticle {
    fn summarize_author(&self) -> String {
        self.author.clone()
    }

    fn summarize(&self) -> String {
        format!("{}, by {} ({})", self.headline, self.author, self.location)
    }
}

impl Summary for Tweet {
    fn summarize_author(&self) -> String {
        format!("@{}", self.username)
    }
    // summarize 使用默认实现
}

// --- 3) trait 作为参数 ---
fn notify(item: &impl Summary) {
    println!("Breaking news! {}", item.summarize());
}

fn notify_bound<T: Summary>(item: &T) {
    println!("Breaking news! {}", item.summarize());
}

// impl：a、b 可以是不同具体类型（各自 impl Summary）
fn notify_two_impl(a: &impl Summary, b: &impl Summary) {
    println!("{} | {}", a.summarize(), b.summarize());
}

// 泛型：a、b 必须是同一类型 T
fn notify_two_same<T: Summary>(a: &T, b: &T) {
    println!("same type: {} | {}", a.summarize(), b.summarize());
}

fn get_msg() -> impl Summary {
    Tweet {
        username: "demo".into(),
        content: "impl Trait return".into(),
        reply: false,
        retweet: false,
    }
}

fn make_adder() -> impl Fn(i32) -> i32 {
    |x| x + 5
}

// --- 常见误区：impl Display（静态）vs &dyn Display（动态）---
fn print_impl(x: impl Display) {
    println!("  impl Display: {x}");
}

fn print_dyn(x: &dyn Display) {
    println!("  dyn Display:  {x}");
}

// --- 4) where 从句 ---
fn print_two<T, U>(t: T, u: U)
where
    T: Display + Clone,
    U: Clone + Debug,
{
    println!("t = {}, u = {:?}", t, u);
}

// --- 5) largest：用 trait bound 修复 ---
fn largest_copy<T: PartialOrd + Copy>(list: &[T]) -> T {
    let mut largest = list[0];
    for &item in list.iter() {
        if item > largest {
            largest = item;
        }
    }
    largest
}

fn largest_clone<T: PartialOrd + Clone>(list: &[T]) -> T {
    let mut largest = list[0].clone();
    for item in list.iter() {
        if item > &largest {
            largest = item.clone();
        }
    }
    largest
}

// 返回引用版本：不需要 Copy/Clone
fn largest_ref<T: PartialOrd>(list: &[T]) -> &T {
    let mut largest = &list[0];
    for item in list.iter() {
        if item > largest {
            largest = item;
        }
    }
    largest
}

// --- 6) 条件实现 Pair<T> ---
#[derive(Debug, Clone)]
struct MyStruct<T> {
    data: T,
}

impl<T> MyStruct<T> {
    fn new(data: T) -> Self {
        Self { data }
    }
}

impl<T> MyStruct<T>
where
    T: Display,
{
    fn print_data(&self) {
        println!("  data = {}", self.data);
    }
}

impl<T: Clone> MyStruct<T> {
    fn dup(&self) -> Self {
        Self {
            data: self.data.clone(),
        }
    }
}

// --- blanket impl 示例：Display → PrintMe ---
trait PrintMe {
    fn print_me(&self);
}

impl<T: Display> PrintMe for T {
    fn print_me(&self) {
        println!("  PrintMe: {self}");
    }
}

// --- 7) 条件实现 Pair<T> ---
#[derive(Debug)]
struct Pair<T> {
    x: T,
    y: T,
}

impl<T> Pair<T> {
    fn new(x: T, y: T) -> Self {
        Self { x, y }
    }
}

impl<T: Display + PartialOrd> Pair<T> {
    fn cmp_display(&self) {
        if self.x >= self.y {
            println!("The largest member is x = {}", self.x);
        } else {
            println!("The largest member is y = {}", self.y);
        }
    }
}

fn main() {
    println!("=== 1) Summary trait + 实现/默认实现 ===");
    let article = NewsArticle {
        headline: "Penguins win the Stanley Cup Championship!".into(),
        location: "Pittsburgh, PA, USA".into(),
        author: "Iceburgh".into(),
        content: "The Pittsburgh Penguins once again are the best hockey team in the NHL.".into(),
    };

    let tweet = Tweet {
        username: "horse_ebooks".into(),
        content: "of course, as you probably already know, people".into(),
        reply: false,
        retweet: false,
    };

    println!("article.summarize() = {}", article.summarize());
    println!("tweet.summarize()   = {}", tweet.summarize()); // 默认实现

    println!("\n=== 2) trait 作为参数 ===");
    notify(&article);
    notify(&tweet);
    notify_bound(&tweet);
    notify_two_impl(&article, &tweet);
    // notify_two_same(&article, &tweet); // ❌ 不同类型，编译失败

    println!("\n=== 3) where 从句 ===");
    print_two("hello".to_string(), vec![1, 2, 3]);

    println!("\n=== 4) largest：Copy vs 引用 ===");
    let nums = vec![34, 50, 25, 100, 65];
    println!("largest_copy(nums) = {}", largest_copy(&nums));

    let words = vec!["aaa".to_string(), "zz".to_string(), "mmmm".to_string()];
    println!("largest_ref(words) = {}", largest_ref(&words));
    println!("largest_clone(words) = {}", largest_clone(&words));

    println!("\n=== 5) 条件 impl：MyStruct — where T: Display 才有 print_data ===");
    let s1 = MyStruct::new(10);
    s1.print_data();
    let s2 = s1.dup();
    println!("dup.data = {}", s2.data);

    println!("\n=== 6) 条件 impl：Pair<T: Display + PartialOrd> 才有 cmp_display ===");
    let p = Pair::new(3, 7);
    p.cmp_display();

    println!("\n=== 7) Blanket impl: Display → PrintMe + ToString ===");
    42.print_me();
    "hello".print_me();
    println!("10.to_string() = {}", 10_i32.to_string());

    println!("\n=== 8) -> impl Trait 返回 ===");
    let msg = get_msg();
    println!("get_msg() = {}", msg.summarize());
    println!("make_adder()(3) = {}", make_adder()(3));

    println!("\n=== 9) impl Display vs dyn Display（静态 vs 动态）===");
    print_impl(42);
    print_impl("hello");
    print_dyn(&42);
    print_dyn(&"hello");
}

