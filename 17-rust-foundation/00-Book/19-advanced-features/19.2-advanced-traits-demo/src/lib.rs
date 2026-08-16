//! 19.2 高级 trait demo：关联类型、默认泛型、FQS、supertrait、newtype

use std::fmt;
use std::ops::Add;

// ── 1. 关联类型 ─────────────────────────────────────────────────────────────

pub trait MyIterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}

#[derive(Debug)]
pub struct Counter {
    cur: u32,
    end: u32,
}

impl Counter {
    pub fn new(end: u32) -> Self {
        Self { cur: 0, end }
    }
}

impl MyIterator for Counter {
    type Item = u32;

    fn next(&mut self) -> Option<Self::Item> {
        if self.cur >= self.end {
            None
        } else {
            self.cur += 1;
            Some(self.cur)
        }
    }
}

/// 带关联类型的 Container（用户笔记 §Container Demo）
pub trait Container {
    type Elem;
    fn push(&mut self, val: Self::Elem);
    fn pop(&mut self) -> Option<Self::Elem>;
}

/// newtype 包装，避免 trait 方法 `push` 与 `Vec::push` 同名递归
pub struct IntBag(Vec<i32>);

impl Container for IntBag {
    type Elem = i32;

    fn push(&mut self, val: Self::Elem) {
        self.0.push(val);
    }

    fn pop(&mut self) -> Option<Self::Elem> {
        self.0.pop()
    }
}

// 泛型 trait 对比：同一类型可多次 impl
pub trait Iterable<T> {
    fn peek(&self) -> Option<T>;
}

pub struct MyIter;

impl Iterable<i32> for MyIter {
    fn peek(&self) -> Option<i32> {
        Some(42)
    }
}

impl Iterable<String> for MyIter {
    fn peek(&self) -> Option<String> {
        Some("hello".into())
    }
}

// ── 2. 默认泛型 + Add ───────────────────────────────────────────────────────

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

impl Add for Point {
    type Output = Point;

    fn add(self, rhs: Point) -> Self::Output {
        Point {
            x: self.x + rhs.x,
            y: self.y + rhs.y,
        }
    }
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct Millimeters(pub u32);

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct Meters(pub u32);

impl Add<Meters> for Millimeters {
    type Output = Millimeters;

    fn add(self, rhs: Meters) -> Self::Output {
        Millimeters(self.0 + rhs.0 * 1000)
    }
}

// ── 3. 完全限定语法 ───────────────────────────────────────────────────────────

pub trait Pilot {
    fn fly(&self);
}

pub trait Wizard {
    fn fly(&self);
}

pub struct Human;

impl Pilot for Human {
    fn fly(&self) {
        println!("  飞行员驾驶飞机");
    }
}

impl Wizard for Human {
    fn fly(&self) {
        println!("  法师御空飞行");
    }
}

impl Human {
    pub fn fly(&self) {
        println!("  人类自己飞行");
    }
}

pub trait Animal {
    fn baby_name() -> String;
}

pub struct Dog;

impl Dog {
    pub fn baby_name() -> String {
        "小狗崽".into()
    }
}

impl Animal for Dog {
    fn baby_name() -> String {
        "幼犬".into()
    }
}

// ── 4. Supertrait ───────────────────────────────────────────────────────────

pub trait OutlinePrint: fmt::Display {
    fn outline_print(&self) {
        let s = self.to_string();
        let len = s.len();
        println!("{}", "*".repeat(len + 2));
        println!("*{}*", s);
        println!("{}", "*".repeat(len + 2));
    }
}

impl fmt::Display for Point {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "({}, {})", self.x, self.y)
    }
}

impl OutlinePrint for Point {}

// ── 5. Newtype ────────────────────────────────────────────────────────────────

pub struct Wrapper(pub Vec<String>);

impl fmt::Display for Wrapper {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[{}]", self.0.join(", "))
    }
}

// ── 整合 demo ───────────────────────────────────────────────────────────────

pub fn demo_associated_types() {
    let mut c = Counter::new(3);
    print!("  Counter: ");
    while let Some(v) = c.next() {
        print!("{v} ");
    }
    println!();

    let mut bag = IntBag(Vec::new());
    bag.push(10);
    bag.push(20);
    println!("  Container IntBag pop: {:?}", bag.pop());
}

pub fn demo_generic_trait_vs_associated() {
    let iter = MyIter;
    println!(
        "  Iterable<i32> peek: {:?}",
        <MyIter as Iterable<i32>>::peek(&iter)
    );
    println!(
        "  Iterable<String> peek: {:?}",
        <MyIter as Iterable<String>>::peek(&iter)
    );
    println!("  → 同一 MyIter 对 Iterable 实现了两次（泛型 trait）");
}

pub fn demo_default_generic_add() {
    let p1 = Point { x: 1, y: 2 };
    let p2 = Point { x: 3, y: 4 };
    let p3 = p1 + p2;
    println!("  Point + Point: {p3:?}");

    let mm = Millimeters(500);
    let m = Meters(2);
    let res = mm + m;
    println!("  Millimeters + Meters: {} mm", res.0);
}

pub fn demo_fully_qualified_syntax() {
    let person = Human;
    person.fly();
    Pilot::fly(&person);
    Wizard::fly(&person);

    println!("  Dog::baby_name = {}", Dog::baby_name());
    println!(
        "  <Dog as Animal>::baby_name = {}",
        <Dog as Animal>::baby_name()
    );
}

pub fn demo_supertrait() {
    let p = Point { x: 1, y: 3 };
    print!("  OutlinePrint: ");
    p.outline_print();
}

pub fn demo_newtype() {
    let w = Wrapper(vec!["a".into(), "b".into(), "c".into()]);
    println!("  Wrapper Display: {w}");
}

pub fn demo_all_advanced_traits() {
    println!("--- §一 关联类型 ---");
    demo_associated_types();
    demo_generic_trait_vs_associated();

    println!("\n--- §二 默认泛型 Add ---");
    demo_default_generic_add();

    println!("\n--- §三 完全限定语法 ---");
    demo_fully_qualified_syntax();

    println!("\n--- §四 supertrait ---");
    demo_supertrait();

    println!("\n--- §五 newtype ---");
    demo_newtype();
}

pub fn demo_compile_error_notes() {
    println!("  【易错 1】impl Container for Vec<i32> 且 fn push 内 self.push(val)");
    println!("    → 无限递归；用 IntBag newtype 委托 self.0.push(val)");
    println!();
    println!("  【易错 2】impl Display for Vec<String>");
    println!("    → E0117 孤儿规则；用 struct Wrapper(Vec<String>)");
    println!();
    println!("  【易错 3】Animal::baby_name() 想调 trait 实现");
    println!("    → 须 <Dog as Animal>::baby_name()");
    println!();
    println!("  【易错 4】impl OutlinePrint for T 但未 impl Display");
    println!("    → supertrait 约束不满足");
    println!();
    println!("  【易错 5】关联类型 vs 泛型 trait");
    println!("    → Container 一次 impl；Iterable<T> 可多次 impl");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn container_push_pop() {
        let mut bag = IntBag(Vec::new());
        bag.push(10);
        assert_eq!(bag.pop(), Some(10));
    }

    #[test]
    fn point_add_default_rhs() {
        assert_eq!(
            Point { x: 1, y: 0 } + Point { x: 2, y: 3 },
            Point { x: 3, y: 3 }
        );
    }

    #[test]
    fn mm_add_meters() {
        assert_eq!(Millimeters(500) + Meters(2), Millimeters(2500));
    }

    #[test]
    fn fqs_animal_baby_name() {
        assert_eq!(Dog::baby_name(), "小狗崽");
        assert_eq!(<Dog as Animal>::baby_name(), "幼犬");
    }

    #[test]
    fn wrapper_display() {
        let w = Wrapper(vec!["x".into()]);
        assert!(w.to_string().contains('x'));
    }
}
