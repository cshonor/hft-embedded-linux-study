// §二 嵌套路径 + §三 父模块/as 别名
use std::{cmp::Ordering, io};
use std::io::{self as std_io, Write};

use use_demo::results;
use use_demo::results::beta::Result as BetaResult;

fn main() {
    // §四 pub use 重导出：外部直接用 use_demo::hosting
    use_demo::eat_at_restaurant();
    use_demo::hosting::add_to_waitlist();
    println!("ok: pub use → use_demo::hosting::add_to_waitlist()");

    // §二 惯用：类型直接导入名；函数保留模块前缀（Ordering 来自 use std::cmp）
    let ordering = Ordering::Equal;
    println!("ordering = {ordering:?}");

    // §五 嵌套路径 self
    let mut out = Vec::<u8>::new();
    write!(&mut out, "hello").unwrap();
    let _ = std_io::stdout();
    let _ = io::stdin();

    // §三 同名冲突：父模块区分 vs as 别名
    let a: results::alpha::Result = Ok("alpha ok");
    let b: BetaResult = Ok(42);
    println!("alpha = {a:?}, beta = {b:?}");

    // §六 glob（限制在小作用域块内）
    {
        use std::collections::*;
        let mut map = HashMap::new();
        map.insert(1, 2);
        println!("glob HashMap len = {}", map.len());
    }

    // §七 跨 Package 示例（本 demo 无外部依赖；见 7.3-cross-package-paths-demo）：
    // use pkg_b::b_mod;
    // pkg_b::b_mod::hello_b();
}
