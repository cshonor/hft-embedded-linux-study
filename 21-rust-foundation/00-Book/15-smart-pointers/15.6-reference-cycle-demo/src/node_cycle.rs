//! 15.6 双向 Node：Rc 环泄漏 vs Weak 修复

use std::cell::RefCell;
use std::rc::{Rc, Weak};

#[derive(Debug)]
pub struct Node {
    pub val: i32,
    pub prev: Option<Rc<RefCell<Node>>>,
}

#[derive(Debug)]
pub struct WeakNode {
    pub val: i32,
    pub prev: Option<Weak<RefCell<WeakNode>>>,
}

/// §二 Rc 互指 → strong 永 ≥ 1（泄漏）
pub fn demo_node_cycle_leak() {
    let a = Rc::new(RefCell::new(Node { val: 1, prev: None }));
    println!("  创建 a → strong(a) = {}", Rc::strong_count(&a));

    let b = Rc::new(RefCell::new(Node {
        val: 2,
        prev: Some(Rc::clone(&a)),
    }));
    println!(
        "  b.prev = a → strong(a) = {}, strong(b) = {}",
        Rc::strong_count(&a),
        Rc::strong_count(&b)
    );

    a.borrow_mut().prev = Some(Rc::clone(&b));
    println!(
        "  成环后 → strong(a) = {}, strong(b) = {}",
        Rc::strong_count(&a),
        Rc::strong_count(&b)
    );
    println!("  出作用域后 strong 仍 ≥ 1 → 内存泄漏（无 UB）");
}

/// §三 prev 改 Weak → 可正常释放
pub fn demo_node_weak_fix() {
    let a = Rc::new(RefCell::new(WeakNode { val: 1, prev: None }));
    let b = Rc::new(RefCell::new(WeakNode {
        val: 2,
        prev: Some(Rc::downgrade(&a)),
    }));
    a.borrow_mut().prev = Some(Rc::downgrade(&b));

    println!(
        "  Weak 修复：strong(a) = {}, strong(b) = {}",
        Rc::strong_count(&a),
        Rc::strong_count(&b)
    );
    println!(
        "  b 能通过 upgrade 看到 a: {:?}",
        b.borrow().prev.as_ref().and_then(|w| w.upgrade()).is_some()
    );
    println!("  出作用域 strong 可归 0 → 正常释放");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn weak_upgrade_some_before_drop() {
        let a = Rc::new(RefCell::new(WeakNode { val: 1, prev: None }));
        let w = Rc::downgrade(&a);
        assert!(w.upgrade().is_some());
    }
}
