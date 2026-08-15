//! 隐式 coercion：类型「弱化」、Trait 对象 unsizing。

trait Speak {
    fn speak(&self) -> &'static str;
}

pub struct Dog;
impl Speak for Dog {
    fn speak(&self) -> &'static str {
        "woof"
    }
}

/// `&String` → `&str`（Deref coercion）
pub fn coerce_string_to_str(s: &String) -> usize {
    takes_str(s)
}

fn takes_str(s: &str) -> usize {
    s.len()
}

/// 具体类型 → trait object（unsizing coercion）
pub fn coerce_to_trait(d: &Dog) -> &'static str {
    takes_speak(d).speak()
}

fn takes_speak(s: &dyn Speak) -> &dyn Speak {
    s
}

/// 定长数组 → 切片引用
pub fn coerce_array_to_slice(arr: &[i32; 3]) -> i32 {
    sum_slice(arr)
}

fn sum_slice(s: &[i32]) -> i32 {
    s.iter().sum()
}

pub fn demo() -> (usize, &'static str, i32) {
    let s = String::from("hello");
    let dog = Dog;
    let arr = [1, 2, 3];
    (
        coerce_string_to_str(&s),
        coerce_to_trait(&dog),
        coerce_array_to_slice(&arr),
    )
}
