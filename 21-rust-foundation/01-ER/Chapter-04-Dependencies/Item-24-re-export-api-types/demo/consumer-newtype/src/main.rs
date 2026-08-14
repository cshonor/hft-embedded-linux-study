fn main() {
    let n = dep_lib_newtype::Picker::pick(10);
    println!("picked {n} (no rand in public API)");
}
