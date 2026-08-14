use nomicon_01_safe_unsafe::{five_powers, privacy, raw_pointers};

fn main() {
    println!("=== raw pointers ===");
    println!("increment = {}", raw_pointers::increment_via_raw_ptr());
    let (before, after) = raw_pointers::const_vs_mut();
    println!("const read={}, after mut write={}", before, after);
    println!("offset_read = {}", raw_pointers::offset_read());

    println!("=== 1+2 raw pointer & unsafe fn ===");
    println!("value = {}", five_powers::raw_pointer_and_unsafe_fn());

    println!("=== 3 unsafe trait ===");
    five_powers::unsafe_trait_demo();

    println!("=== 4 static mut ===");
    println!("counter = {}", five_powers::static_mut_demo());

    println!("=== 5 union field ===");
    println!("union.i = 0x{:x}", five_powers::union_field_demo());

    println!("=== privacy: MiniBuf ===");
    let mut buf = privacy::MiniBuf::with_capacity(4);
    buf.push(b'h');
    buf.push(b'i');
    println!("slice = {:?}", buf.as_slice());
}
