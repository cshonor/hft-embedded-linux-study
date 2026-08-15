//! 15.1 Box demo — 基础 · Cons 链表 · Deref · Drop

use std::ops::Deref;

#[derive(Debug)]
pub enum List {
    Cons(i32, Box<List>),
    Nil,
}

use List::{Cons, Nil};

impl List {
    pub fn len(&self) -> usize {
        match self {
            Cons(_, rest) => 1 + rest.len(),
            Nil => 0,
        }
    }

    pub fn sum(&self) -> i32 {
        match self {
            Cons(head, rest) => head + rest.sum(),
            Nil => 0,
        }
    }
}

pub fn list_1_2_3() -> List {
    Cons(
        1,
        Box::new(Cons(2, Box::new(Cons(3, Box::new(Nil))))),
    )
}

/// §三 基础 Box + Deref
pub fn demo_box_basics() {
    let box_num = Box::new(99);
    assert_eq!(*box_num, 99);
    println!("  box_num = {box_num}（Deref 自动解引用）");

    let b = Box::new(5);
    println!("  b = {b}，*b = {}", *b);
}

/// §二 Cons 链表
pub fn demo_cons_list() {
    let list = list_1_2_3();
    let link = Cons(5, Box::new(Cons(9, Box::new(Nil))));
    println!("  list = {list:?}");
    println!("  link = {link:?}");
    println!("  list len = {}, sum = {}", list.len(), list.sum());
}

/// 带 Drop 日志，观察 Box 出作用域释堆
struct LoudHeap(String);

impl Drop for LoudHeap {
    fn drop(&mut self) {
        println!("  堆上数据 Drop: {:?}", self.0);
    }
}

/// §四 Box + Drop：出作用域释放堆
pub fn demo_box_drop_scope() {
    println!("  --- 进入内层作用域 ---");
    {
        let b = Box::new(LoudHeap(String::from("hello")));
        println!("  使用中: {}", b.0);
    }
    println!("  --- 内层结束，堆已释放 ---");
}

/// 15.1.1：与 `Box` 标准库相同的 `deref` 写法（`&**self`）
struct TeachBox<T>(Box<T>);

impl<T> Deref for TeachBox<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &**&self.0 // self.0 为 &Box<T>，与标准库 deref 体内 &**self 同形
    }
}

/// 模拟标准库 `Box::deref` 方法体：`self` 为 `&Box<T>`
fn peel_box_ref<T>(bx: &Box<T>) -> &T {
    &**bx
}

/// §一 `&**self` / `&*b` / `deref()` 等价演示
pub fn demo_deref_steps() {
    let b = Box::new(10_i32);
    let via_star: &i32 = &*b;
    let via_deref: &i32 = b.deref();
    let via_peel: &i32 = peel_box_ref(&b); // 等价于 impl 内 &**self
    println!("  &*b           → {}", *via_star);
    println!("  b.deref()     → {}", *via_deref);
    println!("  peel(&b)      → {}（= impl 内 &**self）", *via_peel);
    println!("  println!(\"{{}}\", b) → {}", b);

    let t = TeachBox(Box::new(99_i32));
    println!("  TeachBox（impl 内 &**self.0）→ {}", *t.deref());
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cons_list_len_sum() {
        let list = list_1_2_3();
        assert_eq!(list.len(), 3);
        assert_eq!(list.sum(), 6);
    }

    #[test]
    fn box_deref() {
        let b = Box::new(42);
        assert_eq!(*b, 42);
    }

    #[test]
    fn deref_steps_equivalent() {
        let b = Box::new(10_i32);
        assert_eq!(*&*b, 10);
        assert_eq!(*b.deref(), 10);
        assert_eq!(*peel_box_ref(&b), 10);
    }
}
