//! 外部全局变量：仅文档说明，不在此链接真实 C 库。

// 示例声明（勿 uncomment 除非已 link 对应库）：
//
// extern "C" {
//     static mut rl_prompt: *const libc::c_char;
// }
//
// 读写 `static mut` 全局均为 unsafe；多线程下尤甚。

pub fn foreign_globals_note() -> &'static str {
    "see 00-overview.md §5: extern static mut is always unsafe"
}
