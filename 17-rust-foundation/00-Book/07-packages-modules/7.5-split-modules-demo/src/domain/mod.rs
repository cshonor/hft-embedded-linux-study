// pub mod = 登记：把 user.rs / order.rs 挂进 domain 模块树（≠ 给兄弟文件自动 use）
pub mod user;
pub mod order;

// pub use = 收拢：名字上浮到 domain 出口 → 外部 use crate::domain::UserEntity
pub use user::UserEntity;
pub use order::OrderEntity;

pub fn check() {
    let u = UserEntity::new();
    let o = OrderEntity::with_user(&u);
    let _ = (u, o);
}
