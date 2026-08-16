//! §1.1.4 · `FnOnce() -> i32` — stable 可运行写法
//!
//! 手写 `impl FnOnce for MyFunc` 须 nightly `#![feature(fn_traits)]`；
//! 见 1.1.4 §三 场景 2。

fn run<F: FnOnce() -> i32>(f: F) -> i32 {
    f()
}

struct MyFunc;

impl MyFunc {
    fn invoke(self) -> i32 {
        999
    }
}

fn returns_999() -> i32 {
    999
}

pub fn main() {
    assert_eq!(run(|| 999), 999);
    assert_eq!(run(returns_999), 999);
    assert_eq!(run(move || MyFunc.invoke()), 999);
    println!("FnOnce stable demos OK");
}

// --- nightly：FuncObj / FuncObj2 手写 impl（见 1.1.5 §6.2）---
//
// impl FnOnce<()> for FuncObj { type Output = i32; … }      // || 666
// impl FnOnce<(i32,)> for FuncObj2 { type Output = String; … } // |x| x.to_string()
//
// #![feature(fn_traits)]
// #![feature(unboxed_closures)]
//
// struct MyFunc;
//
// impl FnOnce<()> for MyFunc {
//     type Output = i32;
//     extern "rust-call" fn call_once(self, _: ()) -> i32 {
//         999
//     }
// }
//
// fn main() {
//     assert_eq!(run(MyFunc), 999);
//     let _ = std::thread::spawn(MyFunc).join();
// }
