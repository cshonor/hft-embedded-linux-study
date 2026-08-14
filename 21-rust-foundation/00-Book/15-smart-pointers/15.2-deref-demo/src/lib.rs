//! 15.2 Deref / DerefMut demo

use std::ops::{Deref, DerefMut};
use std::sync::Mutex;

/// 教学用：类似 `Box<T>`，数据在结构体内（本节重点在 `Deref` 行为）。
pub struct MyBox<T>(pub T);

impl<T> MyBox<T> {
    pub fn new(x: T) -> Self {
        Self(x)
    }
}

impl<T> Deref for MyBox<T> {
    type Target = T;

    fn deref(&self) -> &T {
        &self.0
    }
}

impl<T> DerefMut for MyBox<T> {
    fn deref_mut(&mut self) -> &mut T {
        &mut self.0
    }
}

pub fn hello(name: &str) {
    println!("Hello, {name}!");
}

/// §二 基础：`*y` ⇔ `*(y.deref())`，coercion `&MyBox<String>` → `&str`
pub fn demo_basic() {
    let x = 5;
    let y = &x;
    assert_eq!(5, *y);

    let y = MyBox::new(5);
    assert_eq!(5, *y);
    assert_eq!(5, *(y.deref()));

    let m = MyBox::new(String::from("Rust"));
    hello(&m);

    let m = MyBox::new(String::from("Rust"));
    hello(&(*m)[..]);
}

/// §三 方法调用自动解引用
pub fn demo_method_call() {
    let b = MyBox::new(String::from("abc"));
    println!("  len = {}（MyBox → &String → str::len）", b.len());
}

/// §四 MutexGuard + Deref
pub fn demo_mutex_guard() {
    let m = Mutex::new(10_i32);
    let guard = m.lock().unwrap();
    println!("  MutexGuard Deref: *guard = {}", *guard);
    println!("  方法调用: guard.abs() = {}", guard.abs());
}

/// §15.2.1 多层 coercion：&Box<MyBox<String>> → &str
pub fn demo_nested_coercion() {
    let nested = Box::new(MyBox::new(String::from("deep")));
    hello(&nested);
    let s: &str = &nested;
    println!("  多层链结果: {s:?}");
}

/// §15.2.1 DerefMut：可变方法 + 索引写
pub fn demo_deref_mut() {
    let mut b = MyBox::new(vec![1, 2, 3]);
    b.push(4);
    (*b)[0] = 99;
    println!("  DerefMut 后 Vec: {:?}", *b);

    let bx = Box::new(String::from("move-out"));
    let s = *bx;
    println!("  Box move-out: {s}");
}

/// §15.2.2 点号两条路线 + &&Box<Rc<String>> 解包链
pub fn demo_dot_two_paths() {
    let s = String::from("hi");
    let r1 = &s;
    let r2 = &&s;
    let r3 = &&&s;
    println!(
        "  原生剥 &: r1.len={}, r2.len={}, r3.len={}（&String 无 Deref trait）",
        r1.len(),
        r2.len(),
        r3.len()
    );

    let b = Box::new(String::from("hi"));
    println!("  Box Deref: b.len={}（Box → &String → 剥 & → len）", b.len());

    use std::rc::Rc;
    let nested = &&Box::new(Rc::new(String::from("deep")));
    println!(
        "  &&Box<Rc<String>>.len() = {}（剥 & → Deref Box → Deref Rc → 剥 & → len）",
        nested.len()
    );
}

/// §15.2.3 普通引用 vs 智能指针 · 关联类型 Target
pub fn demo_ref_vs_smart() {
    let x = 42;
    let rx = &x;
    assert_eq!(*rx, 42);
    println!("  原生 &i32: *rx = {rx:?}（语言剥引用，无 Deref trait）");

    let mut s = String::from("hi");
    {
        let r1 = &mut s;
        r1.push_str("-ref");
    }
    {
        let r2 = Box::new(&mut s);
        r2.push_str("-box"); // &mut Box → DerefMut → &mut String
    }
    println!("  可变借用 .push_str: s = {s:?}");

    let b = Box::new(String::from("Target"));
    let target: &String = b.deref(); // Deref::Target = String
    println!("  Box::deref() → &Target = {target:?}");
    println!("  Box .len() 自动 coercion: len = {}", b.len());
}

/// §15.2.3 自定义 trait 关联类型 + 与 Deref::Target 对照
pub trait Describe {
    type Output;
    fn describe(&self) -> Self::Output;
}

impl Describe for i32 {
    type Output = String;
    fn describe(&self) -> Self::Output {
        format!("number: {self}")
    }
}

pub fn demo_associated_type() {
    let n: i32 = 7;
    let s: String = n.describe();
    println!("  MyTrait 式关联类型: {s}");

    type I32Output = <i32 as Describe>::Output;
    let _: I32Output = s;

    let boxed = MyBox::new(99_i32);
    fn show_deref_target<T>(d: &T)
    where
        T: Deref,
        T::Target: std::fmt::Display,
    {
        let target: &T::Target = d.deref();
        println!("  Deref::Target 显示: {target}");
    }
    show_deref_target(&boxed);
    println!("  Box/MyBox impl Deref {{ type Target = T; }} → *d 面向内部 T");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn nested_coercion_to_str() {
        let nested = Box::new(MyBox::new(String::from("chain")));
        fn takes_str(s: &str) -> usize {
            s.len()
        }
        assert_eq!(takes_str(&nested), 5);
    }

    #[test]
    fn deref_mut_vec_push() {
        let mut b = MyBox::new(vec![1]);
        b.push(2);
        assert_eq!(*b, vec![1, 2]);
    }

    #[test]
    fn mybox_string_coercion() {
        let m = MyBox::new(String::from("hi"));
        assert_eq!(hello_as_len(&m), 2);
    }

    fn hello_as_len(s: &str) -> usize {
        s.len()
    }

    #[test]
    fn dot_two_paths() {
        demo_dot_two_paths();
    }

    #[test]
    fn ref_target_demo() {
        demo_ref_vs_smart();
        demo_associated_type();
    }
}
