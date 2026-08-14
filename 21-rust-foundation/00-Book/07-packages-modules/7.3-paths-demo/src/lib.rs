//! 7.3 路径 demo — 分文件模块树 + 绝对/相对/super/self

pub mod front_of_house;
mod back_of_house;

fn serve_order() {}

pub fn eat_at_restaurant() {
    // 绝对路径（crate 根视角）
    crate::front_of_house::hosting::add_to_waitlist();
    // 相对路径（同级模块名）
    front_of_house::hosting::add_to_waitlist();

    back_of_house::fix_incorrect_order();
    back_of_house::demo_paths_from_back_of_house();
}

pub use back_of_house::{Appetizer, Breakfast};
