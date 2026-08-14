// 11.3 demo：main 只做入口，业务在 lib.rs（否则 tests/ 无法 use main 里的代码）
use test_organization_demo::calc;

fn main() {
    println!("calc(5) = {}", calc(5));
}
