fn main() {
    split_modules_demo::eat_at_restaurant();
    split_modules_demo::hosting::add_to_waitlist();
    split_modules_demo::demo_domain_mod_use();
    split_modules_demo::demo_use_scope_lib_only();
    println!("ok: use 仅当前文件 · pub use 是模块出口");
}

