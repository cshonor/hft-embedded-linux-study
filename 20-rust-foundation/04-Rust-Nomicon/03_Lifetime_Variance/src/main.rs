use nomicon_03_lifetime_variance::{lifetimes, ownership, phantom, split_borrows, variance};

fn main() {
    println!("=== 1 field split borrows (safe) ===");
    println!("Pair = {:?}", ownership::demo());

    println!("=== 3 HRTB ===");
    println!("trimmed = {:?}", lifetimes::demo_hrtb());

    println!("=== 4 variance demo ===");
    variance::demo();

    println!("=== 6 PhantomData ===");
    println!("RawBuf = {:?}", phantom::demo());

    println!("=== 7 split_at_mut ===");
    println!("data[0], data[2] = {:?}", split_borrows::demo());
}
