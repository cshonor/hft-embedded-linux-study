//! Rayon：并行迭代器与 `par_iter`。

use rayon::prelude::*;

pub fn demo() {
    println!("=== rayon ===");

    let v: Vec<i32> = (0..10_000).collect();
    let sum: i32 = v.par_iter().sum();
    println!("par_iter sum(0..10000): {}", sum);

    let max = v.par_iter().max().copied().unwrap_or(0);
    println!("par_iter max: {}", max);
}
