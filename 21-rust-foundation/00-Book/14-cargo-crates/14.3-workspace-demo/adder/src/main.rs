fn main() {
    let num = 10;
    println!(
        "Hello, world! {num} plus one is {}! plus two is {}!",
        add_one::add_one(num),
        add_two::add_two(num)
    );
}
