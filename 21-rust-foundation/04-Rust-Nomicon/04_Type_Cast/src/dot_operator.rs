//! 点运算符：auto-ref、Deref 链、unsizing 找方法。

use std::ops::Deref;

struct LoudString(String);

impl Deref for LoudString {
    type Target = String;
    fn deref(&self) -> &String {
        &self.0
    }
}

impl LoudString {
    fn shout(&self) -> String {
        format!("{}!!!", self.to_uppercase())
    }
}

pub fn demo_deref_and_auto_ref() -> (usize, String) {
    let mut inner = String::from("rust");
    // auto-ref：`push_str` 需要 &mut self
    inner.push_str("nomicon");

    let loud = LoudString(inner);
    // `loud.len()` → Deref 到 String，再 auto-ref
    let len = loud.len();
    // 自定义方法按值接收 &self
    let shouted = loud.shout();
    (len, shouted)
}

pub fn demo_unsizing_method() -> usize {
    let arr = [10, 20, 30];
    // `[T; N]` 通过 unsizing coercion 得到 `&[T]` 以调用 slice 方法
    arr.iter().filter(|x| **x > 15).count()
}
