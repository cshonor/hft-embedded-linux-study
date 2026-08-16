use nomicon_02_data_layout::{exotic, repr_alt, repr_rust};

fn main() {
    println!("target: {}\n", std::env::consts::ARCH);

    repr_rust::print_rust_vs_c();
    println!();
    repr_rust::print_niche();
    println!();

    exotic::print_exotic();
    println!();

    repr_alt::print_alternative_reprs();
}
