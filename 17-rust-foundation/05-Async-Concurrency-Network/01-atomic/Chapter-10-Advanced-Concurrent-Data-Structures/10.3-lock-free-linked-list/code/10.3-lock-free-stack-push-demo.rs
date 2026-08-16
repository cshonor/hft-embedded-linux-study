//! 10.3 无锁栈：头部 `compare_exchange_weak` 入栈（不实现安全回收）。

use std::ptr;
use std::sync::atomic::{AtomicPtr, Ordering};

struct Node {
    data: i32,
    next: *mut Node,
}

pub fn demo() {
    let head = AtomicPtr::new(ptr::null_mut());
    for v in [3, 2, 1] {
        let n = Box::into_raw(Box::new(Node {
            data: v,
            next: ptr::null_mut(),
        }));
        loop {
            let cur = head.load(Ordering::Relaxed);
            unsafe {
                (*n).next = cur;
            }
            if head
                .compare_exchange_weak(cur, n, Ordering::Release, Ordering::Relaxed)
                .is_ok()
            {
                break;
            }
        }
    }
    let mut p = head.load(Ordering::Acquire);
    while !p.is_null() {
        let node = unsafe { &*p };
        print!("{} ", node.data);
        p = node.next;
    }
    println!("\n=== Lock-free stack push (expect 1 2 3) ===");
    // 泄漏节点 — 教学 demo 不实现 hazard pointers
}
