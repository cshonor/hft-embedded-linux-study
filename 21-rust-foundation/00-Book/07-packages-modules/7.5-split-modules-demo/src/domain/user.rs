// 7.4.1：domain/mod.rs 里的 use/pub use 对本文件无效；须在本文件自己 use
use crate::domain::order::OrderEntity;

pub struct UserEntity;

impl UserEntity {
    pub fn new() -> Self {
        let _ = OrderEntity::new();
        Self
    }
}
