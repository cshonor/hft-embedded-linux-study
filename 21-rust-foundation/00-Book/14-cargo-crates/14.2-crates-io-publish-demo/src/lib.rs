//! # Art（示例库）
//!
//! 演示 `//!` crate 级文档、文档测试，以及用 [`pub use`] 把内部模块的类型/函数重导出到 crate 根，便于调用者 `use`。
//!
//! [`pub use`]: https://doc.rust-lang.org/reference/items/use-declarations.html#pub-use-re-export

pub use self::kinds::{PrimaryColor, SecondaryColor};
pub use self::utils::mix;

/// 将输入整数加一。
///
/// # Examples
///
/// ```
/// let arg = 5;
/// let answer = crates_io_publish_demo::add_one(arg);
/// assert_eq!(6, answer);
/// ```
pub fn add_one(x: i32) -> i32 {
    x + 1
}

pub mod kinds {
    //! 颜色类型（RYB 模型示意）。

    /// 三原色（红、黄、蓝）。
    pub enum PrimaryColor {
        Red,
        Yellow,
        Blue,
    }

    /// 三间色（橙、绿、紫）。
    pub enum SecondaryColor {
        Orange,
        Green,
        Purple,
    }
}

pub mod utils {
    //! 与颜色相关的工具函数。

    use crate::kinds::{PrimaryColor, SecondaryColor};

    /// 将两种原色按等量混合，得到对应的间色。
    ///
    /// # Panics
    ///
    /// 若两种颜色相同，或组合不是 RYB 中相邻两原色，则 panic（本示例为教学用简化实现）。
    pub fn mix(c1: PrimaryColor, c2: PrimaryColor) -> SecondaryColor {
        use PrimaryColor::*;
        use SecondaryColor::*;
        match (c1, c2) {
            (Red, Yellow) | (Yellow, Red) => Orange,
            (Yellow, Blue) | (Blue, Yellow) => Green,
            (Blue, Red) | (Red, Blue) => Purple,
            _ => panic!("unsupported primary pair for mix"),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::kinds::{PrimaryColor, SecondaryColor};
    use super::utils::mix;

    #[test]
    fn red_yellow_is_orange() {
        assert!(matches!(
            mix(PrimaryColor::Red, PrimaryColor::Yellow),
            SecondaryColor::Orange
        ));
    }
}
