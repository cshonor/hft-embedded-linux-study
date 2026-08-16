use nomicon_09_ffi::{
    abs_i32_safe, callbacks, globals, interop, nullable, opaque, rust_add, unwind,
};

fn main() {
    println!("=== 1 call C (abs) ===");
    println!("abs(-9) = {}", abs_i32_safe(-9));

    println!("=== 2 export to C ===");
    println!("rust_add(3,4) = {}", rust_add(3, 4));

    println!("=== 3 callbacks ===");
    println!("global cb = {}", callbacks::demo_global_callback());
    println!("stateful cb = {}", callbacks::demo_stateful_callback());

    println!("=== 4 interop ===");
    let p = interop::make_point(3, 4);
    let c = interop::rust_string_to_c("nomicon");
    println!("Point({}, {}), cstr = {:?}", p.x, p.y, c);

    println!("=== 6 nullable fn pointer ===");
    let (v, sz) = nullable::demo_niche();
    println!("Some(triple(4)) = {:?}, size Option<fn> = {} B", v, sz);

    println!("=== 7 catch_unwind ===");
    println!("ok = {:?}", unwind::call_with_catch(1));
    println!("panic caught = {:?}", unwind::call_with_catch(-1));
    println!("C entry err code = {}", unwind::safe_ffi_entry(-1));

    println!("=== 8 opaque ===");
    println!("OpaqueDb size = {} B", std::mem::size_of::<opaque::OpaqueDb>());

    println!("=== 5 foreign globals ===");
    println!("{}", globals::foreign_globals_note());
}
