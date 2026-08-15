//! Safe Rust：分支分析保证「读前必赋」；move 后逻辑未初始化。

/// 所有路径赋值后才读取 — 编译通过。
pub fn conditional_init(flag: bool) -> i32 {
    let x;
    if flag {
        x = 1;
    } else {
        x = 2;
    }
    x
}

/// 非 Copy 类型 move 后，原绑定不可再使用。
pub fn move_leaves_uninit() -> String {
    let s = String::from("moved");
    let t = s;
    // `s` 在此逻辑上未初始化；下一行若写 `s.len()` 则编译失败。
    t
}

// 编译失败示例（勿取消注释）：
// fn read_before_assign() -> i32 {
//     let x: i32;
//     x
// }
