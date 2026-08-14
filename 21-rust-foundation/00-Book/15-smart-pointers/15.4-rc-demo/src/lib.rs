//! 15.4 Rc<T> demo — vs Box · strong_count · 共享尾链表

use std::rc::Rc;

#[derive(Debug)]
pub enum List {
    Cons(i32, Rc<List>),
    Nil,
}

use List::{Cons, Nil};

pub fn list_tail_a() -> Rc<List> {
    Rc::new(Cons(
        5,
        Rc::new(Cons(10, Rc::new(Nil))),
    ))
}

/// §三 共享尾链表（Book 15-18 / 15-19）
pub fn demo_shared_tail_list() {
    let a = list_tail_a();
    println!("  count after creating a = {}", Rc::strong_count(&a));

    let b = Cons(3, Rc::clone(&a));
    println!("  count after creating b = {}", Rc::strong_count(&a));

    {
        let c = Cons(4, Rc::clone(&a));
        println!("  count after creating c = {}", Rc::strong_count(&a));
        println!("  c = {c:?}");
    }

    println!(
        "  count after c goes out of scope = {}",
        Rc::strong_count(&a)
    );
    println!("  b = {b:?}");
}

/// §二 Rc<i32> 计数逐步演示
pub fn demo_rc_count_steps() {
    let a = Rc::new(10);
    println!("  创建 a → count = {}", Rc::strong_count(&a));

    let b = Rc::clone(&a);
    println!("  Rc::clone → b → count = {}", Rc::strong_count(&a));

    let c = Rc::clone(&a);
    println!("  再 clone → c → count = {}", Rc::strong_count(&a));
    println!("  a={a}, b={b}, c={c}");

    {
        let d = Rc::clone(&a);
        println!("  作用域内 d → count = {}", Rc::strong_count(&a));
    }
    println!("  d drop 后 → count = {}", Rc::strong_count(&a));
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn clone_increments_strong_count() {
        let a = Rc::new(42);
        assert_eq!(Rc::strong_count(&a), 1);
        let _b = Rc::clone(&a);
        assert_eq!(Rc::strong_count(&a), 2);
    }

    #[test]
    fn shared_tail_keeps_a_alive() {
        let a = list_tail_a();
        let _b = Cons(3, Rc::clone(&a));
        assert!(Rc::strong_count(&a) >= 2);
    }
}

// Box 独占：move 后原变量失效（编译失败示例，勿取消注释）
// fn box_not_shareable() {
//     let a = Box::new(10);
//     let _b = a;
//     println!("{a}");
// }
