//! 7.4 use demo — pub use 重导出 + 惯用导入规范

mod front_of_house {
    pub mod hosting {
        pub fn add_to_waitlist() {}
    }
}

// pub use：外部可 use use_demo::hosting（不必知道 front_of_house 内部结构）
pub use crate::front_of_house::hosting;

pub fn eat_at_restaurant() {
    // 惯用：导入父模块后 模块::函数()
    hosting::add_to_waitlist();
}

// 同名冲突演示：两个 Result 类型
pub mod results {
    pub mod alpha {
        pub type Result = std::result::Result<&'static str, &'static str>;
    }

    pub mod beta {
        pub type Result = std::result::Result<u32, &'static str>;
    }
}
