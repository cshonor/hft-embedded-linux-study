pub mod front_of_house;
pub mod domain;

pub use crate::front_of_house::hosting;

// 7.4.1：此 use 仅 lib.rs 本文件有效，domain/ 子模块不继承
use std::collections::HashMap;

pub fn eat_at_restaurant() {
    hosting::add_to_waitlist();
    front_of_house::serving::take_order();
}

pub fn demo_domain_mod_use() {
    domain::check();
    let _u = domain::UserEntity::new(); // 7.4 pub use 重导出 → domain 模块出口
}

pub fn demo_use_scope_lib_only() {
    let _m = HashMap::<(), ()>::new(); // HashMap 短名只在本文件可用
}
