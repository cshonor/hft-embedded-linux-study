use bin_plus_lib_demo::math::add;

fn main() {
    println!("{}", add(1, 2));
    let msg = bin_plus_lib_demo::greet("src/main.rs (binary crate)");
    println!("{msg}");
    println!("{}", bin_plus_lib_demo::via_lib2());
}
