//! Item 15: borrow checker — mem::replace, 缩短作用域, 临时值绑定

use std::mem;

fn main() {
    println!("=== mem::replace on &mut Option ===");
    let mut slot: Option<String> = Some("old".into());
    #[allow(clippy::mem_replace_option_with_some)] // 演示 mem::replace；日常可用 Option::replace
    let previous = mem::replace(&mut slot, Some("new".into()));
    println!("previous = {:?}, slot = {:?}", previous, slot);

    println!("\n=== 延长临时值生命周期 ===");
    let query = format!("prefix-{}", 42);
    let len = find_len(&query, "fix");
    println!("len = {len}");

    println!("\n=== 缩短借用作用域 ===");
    let mut v = vec![1, 2, 3];
    {
        let first = &v[0];
        println!("borrowed first = {first}");
    }
    v.push(4);
    println!("after push: {v:?}");
}

fn find_len(haystack: &str, needle: &str) -> usize {
    haystack
        .find(needle)
        .map(|i| haystack[i..].len())
        .unwrap_or(0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn replace_swaps_option() {
        let mut o = Some(1u32);
        let old = mem::replace(&mut o, Some(2));
        assert_eq!(old, Some(1));
        assert_eq!(o, Some(2));
    }
}
