//! §9.3：`rayon` 并行求和。
use rayon::prelude::*;

fn main() {
    let data: Vec<i32> = (0..10_000).collect();
    let seq: i32 = data.iter().map(|x| x * x).sum();
    let par: i32 = data.par_iter().map(|x| x * x).sum();
    assert_eq!(seq, par);
    println!("rayon par_iter sum of squares (0..10000): {par}");
}
