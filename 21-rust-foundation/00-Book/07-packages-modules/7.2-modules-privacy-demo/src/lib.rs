//! 7.2 模块树 demo — lib.rs = lib crate root = 模块树顶层 `crate`

mod front_of_house {
    pub mod hosting {
        pub fn add_to_waitlist() {
            seat_at_table();
        }

        fn seat_at_table() {}
    }

    mod serving {}
}

pub mod user_mod;

pub fn eat_at_restaurant() {
    front_of_house::hosting::add_to_waitlist();
}
