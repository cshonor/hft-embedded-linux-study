// 8.3.4 Hasher / BuildHasher demo

use std::collections::HashMap;
use std::collections::hash_map::{DefaultHasher, RandomState};
use std::hash::{BuildHasher, Hash, Hasher};
use std::hash::BuildHasherDefault;
use fnv::FnvHasher;

type FnvHashMap<K, V> = HashMap<K, V, BuildHasherDefault<FnvHasher>>;

fn hash_key<B: BuildHasher + Default, T: Hash>(key: &T) -> u64 {
    let mut hasher = B::default().build_hasher();
    key.hash(&mut hasher);
    hasher.finish()
}

fn main() {
    println!("=== 1) RandomState → DefaultHasher (SipHash) ===");
    let sip = hash_key::<RandomState, _>(&42u32);
    println!("u32 42 SipHash = {sip}");

    println!("\n=== 2) 直接用 DefaultHasher ===");
    let mut h = DefaultHasher::new();
    42u32.hash(&mut h);
    println!("u32 42 DefaultHasher::finish = {}", h.finish());

    println!("\n=== 3) FnvHashMap 别名 ===");
    let mut map: FnvHashMap<u64, &str> = FnvHashMap::default();
    map.insert(1, "alpha");
    map.insert(2, "beta");
    println!("FnvHashMap = {:?}", map);

    let fnv = hash_key::<BuildHasherDefault<FnvHasher>, _>(&42u32);
    println!("u32 42 FNV hash = {fnv}（算法不同 → 与 SipHash 值不同）");

    println!("\n=== 4) 默认 HashMap S = RandomState ===");
    let mut safe: HashMap<u32, i32> = HashMap::new();
    safe.insert(100, 1);
    println!("默认 HashMap = {:?}", safe);

    println!("\nok: Hasher 单次计算 · BuildHasher 工厂 · 可换 Fnv");
}
