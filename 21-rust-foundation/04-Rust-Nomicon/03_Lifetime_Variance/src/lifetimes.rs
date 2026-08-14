//! 生命周期省略、HRTB、无界 lifetime（unsafe raw ptr）。

/// Elision：单输入 → 输出与输入同 lifetime（规则 1）。
pub fn first_word(s: &str) -> &str {
    s.split_whitespace().next().unwrap_or("")
}

/// HRTB：闭包须对**任意** `'a` 的 `&str` 都合法。
pub fn apply_hrtb<F>(f: F, s: &str) -> &str
where
    F: for<'a> Fn(&'a str) -> &'a str,
{
    f(s)
}

/// 协变传递：`'long: 'short` 时，`&'long T` 可当作 `&'short T`。
pub fn shorten<'short>(s: &'short str) -> &'short str {
    let owned = String::from("temporary string buffer");
    let long: &str = &owned;
    // 若 return long，则编译失败：owned 在函数结束时 drop。
    let _ = long;
    s
}

/// 无界 lifetime 警示：解引用 raw ptr 得到的引用生命周期不可盲信。
pub unsafe fn deref_raw(ptr: *const i32) -> &'static i32 {
    // Nomicon：此类引用 lifetime 可能「无界膨胀」；生产代码应绑定到明确 owner。
    &*ptr
}

pub fn demo_hrtb() -> &'static str {
    apply_hrtb(|s| s.trim(), "  hello  ")
}
