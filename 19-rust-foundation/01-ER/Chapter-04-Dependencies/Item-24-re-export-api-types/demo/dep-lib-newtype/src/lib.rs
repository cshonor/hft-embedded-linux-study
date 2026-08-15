//! Item 24 拓展：newtype 隐藏 rand，API 不暴露公开依赖

use rand::Rng;

pub struct Picker;

impl Picker {
    pub fn pick(max: u32) -> u32 {
        let mut rng = rand::thread_rng();
        rng.gen_range(0..max)
    }
}
