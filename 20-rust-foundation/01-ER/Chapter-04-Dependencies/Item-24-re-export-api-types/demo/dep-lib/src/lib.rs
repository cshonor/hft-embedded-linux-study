//! 中间库：API 暴露 rand 0.8 的类型，并重导出

pub use rand;

pub fn pick_number_with<R: rand::Rng>(rng: &mut R, max: u32) -> u32 {
    rng.gen_range(0..max)
}
