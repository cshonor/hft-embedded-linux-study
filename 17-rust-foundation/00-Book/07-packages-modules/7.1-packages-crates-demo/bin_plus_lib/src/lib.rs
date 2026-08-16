pub mod math;

mod lib2; // src/lib2.rs = 同 crate 内模块，不是第二个 Library Crate

pub fn greet(from: &str) -> String {
    format!("hello from library crate: {from}")
}

pub fn via_lib2() -> &'static str {
    lib2::helper()
}
