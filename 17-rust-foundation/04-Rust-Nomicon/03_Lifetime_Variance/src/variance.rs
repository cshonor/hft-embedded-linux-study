//! 型变直觉：`&T` 协变，`&mut T` 对 T 不变。

/// `&'a str` 对 `'a` 协变：较长 lifetime 可降级为较短。
pub fn covariant_lifetime<'long: 'short, 'short>(long: &'long str) -> &'short str {
    long
}

/// `&T` 对 T 协变（通过子类型）：`&'a String` 可当作 `&'a str` 使用（Deref  coercion 场景）。
pub fn covariant_type(s: &String) -> &str {
    s.as_str()
}

/// `&mut T` 对 T **不变**：不能把 `&mut String` 当成 `&mut str` 传递。
/// 下列函数若写成接受 `&mut str` 并传入 `&mut String` 会编译失败 — 防 UAF。
pub fn invariant_mut(x: &mut u32) -> &mut u32 {
    x
}

/// 逆变直觉：若 `Dog <: Animal`，则 `fn(Animal)` 是 `fn(Dog)` 的子类型（参数方向相反）。
pub fn use_contravariant(_cb: fn(&str)) {}

pub fn demo() {
    let mut n = 1u32;
    invariant_mut(&mut n);
    use_contravariant(|_| {});
}
