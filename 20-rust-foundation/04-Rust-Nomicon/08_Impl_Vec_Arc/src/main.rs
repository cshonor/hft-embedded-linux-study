use nomicon_08_impl_vec::{MyArc, MyVec};
use std::thread;

fn main() {
    demo_vec();
    println!();
    demo_arc();
}

fn demo_vec() {
    println!("=== push / pop / Deref ===");
    let mut v = MyVec::new();
    v.push(10);
    v.push(20);
    v.push(30);
    println!("slice = {:?}", &v[..]);
    println!("pop = {:?}", v.pop());

    println!("=== insert / remove ===");
    v.insert(1, 15);
    println!("after insert = {:?}", &v[..]);
    println!("remove(1) = {}", v.remove(1));
    println!("after remove = {:?}", &v[..]);

    println!("=== IntoIter ===");
    let v2 = MyVec::from_iter(vec![1, 2, 3]);
    let sum: i32 = v2.into_iter().sum();
    println!("sum = {}", sum);

    println!("=== Drain ===");
    let mut v3 = MyVec::from_iter(vec![1, 2, 3, 4]);
    let mid: Vec<_> = v3.drain(1..3).collect();
    println!("drained = {:?}, rest = {:?}", mid, &v3[..]);

    println!("=== ZST ===");
    struct Nothing;
    let mut z: MyVec<Nothing> = MyVec::new();
    z.push(Nothing);
    z.push(Nothing);
    println!("zst len = {}", z.len());
    z.pop();
    println!("zst len after pop = {}", z.len());
}

fn demo_arc() {
    println!("=== MyArc new / Deref / Clone ===");
    let a = MyArc::new(100);
    let b = a.clone();
    println!("*a = {}, strong_count = {}", *a, a.strong_count());
    println!("*b = {}, strong_count = {}", *b, b.strong_count());

    println!("=== MyArc cross-thread ===");
    let shared = MyArc::new(vec!["alpha", "beta"]);
    let worker = shared.clone();
    let handle = thread::spawn(move || worker[1].to_string());
    println!("main sees {}", shared[0]);
    println!("thread got {}", handle.join().unwrap());
    println!("final strong_count = {}", shared.strong_count());
}
