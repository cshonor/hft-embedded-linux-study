//! 15.5 RefCell / Rc 组合 demo

use std::cell::{Cell, RefCell};
use std::rc::Rc;

#[derive(Debug)]
#[allow(dead_code)]
pub enum List {
    Cons(Rc<RefCell<i32>>, Rc<List>),
    Nil,
}

use List::{Cons, Nil};

/// §三 Rc<Cell<i32>>
pub fn demo_rc_cell() {
    let num = Rc::new(Cell::new(10));
    let n1 = Rc::clone(&num);
    let n2 = Rc::clone(&num);
    n1.set(20);
    println!("  Rc<Cell>: n2.get() = {}（n1.set(20) 后）", n2.get());
}

/// §四 Rc<RefCell<String>>
pub fn demo_rc_refcell_string() {
    let s = Rc::new(RefCell::new(String::from("hello")));
    let s1 = Rc::clone(&s);
    let s2 = Rc::clone(&s);
    s1.borrow_mut().push_str(" world");
    println!("  Rc<RefCell>: s2.borrow() = {}", s2.borrow());
}

/// §四 Book 共享尾链表 + 修改共享 value
pub fn demo_rc_refcell_list() {
    let value = Rc::new(RefCell::new(5));
    let a = Rc::new(Cons(Rc::clone(&value), Rc::new(Nil)));
    let b = Cons(Rc::new(RefCell::new(6)), Rc::clone(&a));
    let c = Cons(Rc::new(RefCell::new(10)), Rc::clone(&a));

    *value.borrow_mut() += 10;

    println!("  value 改为 15 后:");
    println!("    a = {a:?}");
    println!("    b = {b:?}");
    println!("    c = {c:?}");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rc_cell_shared_mutate() {
        let n = Rc::new(Cell::new(1));
        Rc::clone(&n).set(99);
        assert_eq!(n.get(), 99);
    }

    #[test]
    fn rc_refcell_borrow_mut() {
        let s = Rc::new(RefCell::new(String::from("x")));
        Rc::clone(&s).borrow_mut().push('y');
        assert_eq!(*s.borrow(), "xy");
    }
}
