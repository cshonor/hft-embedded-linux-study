//! Item 5/6：Deref 强制转换 + derive_more newtype + AsRef/Into + 过度 Deref 反例

mod as_ref_vs_into;
mod over_deref;

use as_ref_vs_into::{greet, greet_owned, store_label};
use derive_more::{Add, Display, From};
use over_deref::{LeakyDb, UserDb, UserId};
use std::ops::Deref;

/// derive_more：减少 `.0` 样板
#[derive(Debug, Clone, Copy, PartialEq, From, Add, Display)]
struct Meters(u32);

/// 手动 Deref：透明访问内部（Item 5 — 方法解析会尝试 deref）
struct Wrapper(String);

impl Deref for Wrapper {
    type Target = str;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

fn print_len(s: &str) {
    println!("len = {}", s.len());
}

fn main() {
    let a = Meters(3);
    let b = Meters(5);
    println!("distance = {} m", a + b);

    let w = Wrapper("hello".into());
    print_len(&w); // &Wrapper → &str via Deref coercion
    println!("deref str: {}", w.to_uppercase());

    greet("Item 05");
    greet_owned(String::from("into-demo")); // 反例：只读却取得所有权
    let stored = store_label("as-ref-vs-into");
    println!("stored = {stored}");

    let mut good = UserDb::new();
    good.insert(UserId::new(1), "alice");
    println!("UserDb: {:?}", good.get(UserId::new(1)));

    let mut leaky = LeakyDb::new();
    leaky.insert(UserId::new(0), String::new()); // 绕过校验 —— 见 over_deref 模块文档
    println!("LeakyDb bypassed validation (empty name allowed)");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn meters_add() {
        assert_eq!(Meters(1) + Meters(2), Meters(3));
    }

    #[test]
    fn wrapper_coerces_to_str() {
        let w = Wrapper("ab".into());
        assert_eq!(w.len(), 2);
    }
}
