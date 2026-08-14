use dep_lib::pick_number_with;

fn main() {
    // 正确：使用 dep-lib 重导出的 rand 0.8
    let mut rng = dep_lib::rand::thread_rng();
    let n = pick_number_with(&mut rng, 10);
    println!("picked {n}");
}
