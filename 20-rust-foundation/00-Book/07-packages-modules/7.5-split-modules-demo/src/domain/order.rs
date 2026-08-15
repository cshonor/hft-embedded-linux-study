// 7.5.2：mod.rs 的 pub mod user 不会帮本文件自动导入；须手写 use
use crate::domain::user::UserEntity;

pub struct OrderEntity;

impl OrderEntity {
    pub fn new() -> Self {
        Self
    }

    pub fn with_user(_user: &UserEntity) -> Self {
        Self
    }
}
