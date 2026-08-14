// 10.2.2 Trait 七种写法 — 可运行对比 demo

use std::fmt::Debug;

// ── §1/§2 Speak ──────────────────────────────────────
trait Speak {
    fn hello(&self);
}

struct Dog;
struct Cat;

impl Speak for Dog {
    fn hello(&self) {
        println!("  汪汪");
    }
}

impl Speak for Cat {
    fn hello(&self) {
        println!("  喵喵");
    }
}

fn test_impl(a: impl Speak, b: impl Speak) {
    a.hello();
    b.hello();
}

fn test_generic<T: Speak>(a: T, b: T) {
    a.hello();
    b.hello();
}

// ── §3/§4 Run + Jump ─────────────────────────────────
trait Run {}
trait Jump {}

struct Rabbit;

impl Run for Rabbit {}
impl Jump for Rabbit {}

impl Run for Dog {}

fn go(x: impl Run + Jump) {
    let _ = x;
    println!("  Rabbit: Run + Jump OK");
}

fn go_where<T, U>(a: T, b: U)
where
    T: Run + Jump,
    U: Speak + Run,
{
    let _ = (a, b);
    println!("  where: T=Run+Jump, U=Speak+Run OK");
}

// ── §5 返回 impl Fruit ───────────────────────────────
trait Fruit {
    fn name(&self) -> &str;
}

struct Apple;

impl Fruit for Apple {
    fn name(&self) -> &str {
        "苹果"
    }
}

struct Banana;

impl Fruit for Banana {
    fn name(&self) -> &str {
        "香蕉"
    }
}

fn get_fruit() -> impl Fruit {
    Apple
}

// §8：运行期分支多种类型 → dyn
fn pick_fruit(flag: bool) -> Box<dyn Fruit> {
    if flag {
        Box::new(Apple)
    } else {
        Box::new(Banana)
    }
}

// ── §6 条件 impl ─────────────────────────────────────
struct Data<T>(T);

trait Calc {}

impl Calc for i32 {}

impl<T: Calc> Data<T> {
    fn add_label(&self) {
        println!("  Data<Calc>.add_label OK");
    }
}

// ── §7 Blanket impl Show ─────────────────────────────
trait Show {
    fn show(&self);
}

impl<T: Debug> Show for T {
    fn show(&self) {
        println!("  Show: {:?}", self);
    }
}

fn main() {
    println!("=== §1 impl Speak: Dog + Cat（不同类型）===");
    test_impl(Dog, Cat);

    println!("\n=== §2 泛型 T: Speak: Dog + Dog（同类型）===");
    test_generic(Dog, Dog);
    // test_generic(Dog, Cat); // ❌ 编译报错

    println!("\n=== §3 impl Run + Jump ===");
    go(Rabbit);

    println!("\n=== §4 where T: Run+Jump, U: Speak+Run ===");
    go_where(Rabbit, Dog);

    println!("\n=== §5 -> impl Fruit（固定 Apple）===");
    println!("  fruit = {}", get_fruit().name());

    println!("\n=== §8 impl vs dyn：分支返回 Apple/Banana ===");
    println!("  flag=true  → {}", pick_fruit(true).name());
    println!("  flag=false → {}", pick_fruit(false).name());
    // fn bad(flag: bool) -> impl Fruit { if flag { Apple } else { Banana } } // ❌

    println!("\n=== §6 impl<T: Calc> Data<T> ===");
    Data(42i32).add_label();

    println!("\n=== §7 Blanket impl<T: Debug> Show for T ===");
    10.show();
    "hello".show();

    println!("\n=== bonus：ToString blanket（标准库）===");
    println!("  10.to_string() = {}", 10_i32.to_string());

    println!("\nok: 七种写法 demo 完成");
}
