//! crossbeam-channel + crossbeam_utils::thread::scope
//!
//! 避免与模块名 `crossbeam` 混淆：依赖 crate 仍为 `crossbeam-channel` / `crossbeam-utils`。

use crossbeam_channel::bounded;
use crossbeam_utils::thread::scope;

pub fn demo() {
    println!("=== crossbeam（channel + scope）===");

    let (s, r) = bounded::<i32>(4);
    scope(|sp| {
        sp.spawn(|_| {
            for i in 0..3 {
                s.send(i).expect("send");
            }
            drop(s);
        });
        sp.spawn(|_| {
            while let Ok(v) = r.recv() {
                println!("  channel 收到: {}", v);
            }
        });
    })
    .unwrap();

    println!("scope 内子线程已 join 完成");
}
