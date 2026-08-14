use nomicon_04_type_cast::{casts, coercions, dot_operator, transmute};

fn main() {
    println!("=== 1 coercions ===");
    println!("{:?}", coercions::demo());

    println!("=== 2 dot operator ===");
    println!("deref/auto-ref = {:?}", dot_operator::demo_deref_and_auto_ref());
    println!("unsizing filter count = {}", dot_operator::demo_unsizing_method());

    println!("=== 3 casts ===");
    println!("numeric + ptr chain = {:?}", casts::demo());
    println!("non-transitivity: {}", casts::demo_non_transitivity_note());

    println!("=== 4 transmute (same size only) ===");
    println!("u32 bytes = {:02x?}", transmute::demo());
}
