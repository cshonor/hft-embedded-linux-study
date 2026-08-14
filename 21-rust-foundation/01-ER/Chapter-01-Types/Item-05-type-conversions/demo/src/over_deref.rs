//! Item 6：**过度 Deref** 反例（文档性）
//!
//! 把 newtype 完全透明代理成内部类型时，调用方会绕过你设计的校验 API，
//! 直接拿到 `HashMap` 的全部方法 —— 类型边界被 Deref 模糊化。
//!
//! **更稳妥**：只暴露受控方法（`get_user` / `insert_user`），或仅 `Deref` 到
//! 更窄的 trait（如 `AsRef<[Entry]>`），而不是整个内部容器。

use std::collections::HashMap;
use std::ops::{Deref, DerefMut};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct UserId(u64);

impl UserId {
    pub fn new(id: u64) -> Self {
        Self(id)
    }
}

/// 带校验的「理想」API
pub struct UserDb {
    inner: HashMap<UserId, String>,
}

impl UserDb {
    pub fn new() -> Self {
        Self {
            inner: HashMap::new(),
        }
    }

    pub fn insert(&mut self, id: UserId, name: impl Into<String>) {
        let name = name.into();
        assert!(!name.is_empty(), "name must be non-empty");
        self.inner.insert(id, name);
    }

    pub fn get(&self, id: UserId) -> Option<&str> {
        self.inner.get(&id).map(String::as_str)
    }
}

/// 反例：Deref 到整个 `HashMap`，任何 `db.insert(UserId(0), "".into())` 都能绕过校验
pub struct LeakyDb {
    inner: HashMap<UserId, String>,
}

impl LeakyDb {
    pub fn new() -> Self {
        Self {
            inner: HashMap::new(),
        }
    }
}

impl Deref for LeakyDb {
    type Target = HashMap<UserId, String>;
    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl DerefMut for LeakyDb {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.inner
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn controlled_api_rejects_empty_name() {
        let mut db = UserDb::new();
        db.insert(UserId::new(1), "alice");
        assert_eq!(db.get(UserId::new(1)), Some("alice"));
    }

    #[test]
    #[should_panic(expected = "name must be non-empty")]
    fn controlled_api_panics_on_empty() {
        let mut db = UserDb::new();
        db.insert(UserId::new(1), "");
    }

    #[test]
    fn leaky_deref_bypasses_validation() {
        let mut db = LeakyDb::new();
        // 方法解析：`LeakyDb` → `&mut HashMap` via DerefMut，校验被完全绕过
        db.insert(UserId::new(99), String::new());
        assert!(db.get(&UserId::new(99)).unwrap().is_empty());
    }
}
