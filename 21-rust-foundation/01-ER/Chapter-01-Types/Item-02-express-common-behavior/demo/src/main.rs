//! Item 2: 回调与泛型设计 — 四条结论可运行示例
//!
//! 对应笔记：../03-key-takeaways.md

use std::io::{Cursor, Read, Result};

// ── 1. 回调 API 优先 Fn*，少用裸 fn ─────────────────────────────

fn call_raw_fn(f: fn(i32) -> i32, x: i32) -> i32 {
    f(x)
}

fn call_callback<F: Fn(i32) -> i32>(f: F, x: i32) -> i32 {
    f(x)
}

fn add_one(x: i32) -> i32 {
    x + 1
}

fn demo_fn_vs_closure() {
    println!("=== 1. fn vs Fn* ===");
    println!("裸函数指针: {}", call_raw_fn(add_one, 10));

    let offset = 5;
    let closure = |x| x + offset;
    // call_raw_fn(closure, 10); // ❌ 闭包不能转为 fn
    println!("Fn 闭包回调: {}", call_callback(closure, 10));
}

// ── 2. 选择最宽的 Fn 约束 ─────────────────────────────────────

fn run_once<F: FnOnce()>(f: F) {
    f();
}

fn run_mut<F: FnMut(i32)>(mut f: F) {
    f(1);
    f(2);
}

fn run_normal<F: Fn(i32)>(f: F) {
    f(1);
    f(2);
}

fn demo_fn_constraints() {
    println!("\n=== 2. FnOnce / FnMut / Fn ===");

    let s = String::from("hello");
    run_once(move || println!("FnOnce move: {s}"));

    let mut cnt = 0;
    run_mut(|x| cnt += x);
    println!("FnMut 计数: {cnt}");

    let num = 100;
    run_normal(|x| println!("Fn 只读: {x} + {num} = {}", x + num));
}

// ── 3. 优先 Trait bound，不绑死具体类型 ───────────────────────

fn read_data<R: Read>(mut reader: R) -> Result<String> {
    let mut buf = String::new();
    reader.read_to_string(&mut buf)?;
    Ok(buf)
}

fn demo_trait_bound() -> Result<()> {
    println!("\n=== 3. R: Read 而非 File ===");

    let mock = b"mock content for unit test";
    let text = read_data(Cursor::new(mock))?;
    println!("从内存读取: {text:?}");

    Ok(())
}

// ── 4. 裸泛型 T 能力有限 ───────────────────────────────────────

fn bare_generic<T>(val: T) {
    // println!("{val}");  // ❌ 需要 T: Display
    let _moved = val;
}

fn display_generic<T: std::fmt::Display>(val: T) {
    println!("Display 约束: {val}");
}

fn demo_bare_generic() {
    println!("\n=== 4. 裸 T vs T: Display ===");
    bare_generic(123);
    display_generic(456);
    display_generic("rust");
}

fn main() -> Result<()> {
    demo_fn_vs_closure();
    demo_fn_constraints();
    demo_trait_bound()?;
    demo_bare_generic();
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn read_from_cursor() {
        let s = read_data(Cursor::new(b"hi")).unwrap();
        assert_eq!(s, "hi");
    }
}
