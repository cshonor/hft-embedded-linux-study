fn main() {
    // 跨 Package：只用 包名:: 绝对路径，没有 super::
    pkg_b::b_mod::hello_b();

    let d = pkg_b::b_mod::Data { val: 1 };
    println!("ok: pkg_a → pkg_b::b_mod::hello_b(), Data.val = {}", d.val);
    // super::pkg_b::b_mod::hello_b();  // ❌ super 只在同 crate 内有效
}
