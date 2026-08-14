//! Item 5：`AsRef` vs `Into` — API 边界设计
//!
//! | 场景 | 倾向 | 原因 |
//! |------|------|------|
//! | 只读、可借 | `impl AsRef<str>` | 调用方保留所有权；`&str`/`String`/`Cow` 都能传 |
//! | 要存进结构体 | `impl Into<String>` | 一次性取得所有权，避免内部再 clone |
//! | 库边界泛化 | `AsRef` 优先 | 少分配；需要拥有时再在内部 `into()` |

/// 只读：打印问候 —— 接受任何可借为 `&str` 的类型
pub fn greet(name: impl AsRef<str>) {
    println!("Hello, {}", name.as_ref());
}

/// 写入：需要持久化 —— 取得 `String` 所有权
pub fn store_label(label: impl Into<String>) -> String {
    label.into()
}

/// 反例：同时要求 `Into<String>` 却只读 —— 迫使调用方放弃所有权或多余 clone
pub fn greet_owned(label: impl Into<String>) {
    println!("label = {}", label.into());
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn as_ref_accepts_borrowed_and_owned() {
        greet("alice");
        greet(String::from("bob"));
    }

    #[test]
    fn into_stores_without_extra_clone_from_str() {
        let s = store_label("tag");
        assert_eq!(s, "tag");
        let owned = store_label(String::from("owned"));
        assert_eq!(owned, "owned");
    }

    #[test]
    fn into_for_read_only_forces_move() {
        let label = String::from("tag");
        greet_owned(label);
        // `label` moved — 只读场景应优先 `AsRef`（见 `greet`）
    }
}
