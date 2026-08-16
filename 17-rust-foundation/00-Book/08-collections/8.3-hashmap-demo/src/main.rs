// 8.3 HashMap<K, V> 完整 demo

use std::collections::HashMap;

fn main() {
    println!("=== 1) new + insert + 覆盖返回旧值 ===");
    let mut scores = HashMap::new();
    scores.insert(String::from("Blue"), 10);
    scores.insert(String::from("Yellow"), 50);
    let old = scores.insert(String::from("Blue"), 25);
    println!("覆盖 Blue: old={:?}, scores={:?}", old, scores);

    println!("\n=== 2) zip + collect / from / with_capacity ===");
    // iter+zip → (&String,&i32)；into_iter+zip → (String,i32) → 见 8.3.1
    let keys = vec![String::from("蓝"), String::from("黄")];
    let vals = vec![10, 50];
    let ref_map: HashMap<_, _> = keys.iter().zip(vals.iter()).collect();
    println!("iter().zip().collect → {:?}", ref_map);

    let teams = vec![String::from("A"), String::from("B")];
    let nums = vec![10, 20];
    let mut zip_map: HashMap<String, i32> = teams.into_iter().zip(nums).collect();
    println!("into_iter().zip().collect → {:?}", zip_map);

    let from_map = HashMap::from([("Blue", 10), ("Yellow", 50)]);
    let _pre = HashMap::<&str, i32>::with_capacity(10);
    println!("from = {:?}", from_map);

    println!("\n=== 3) [] / get / get_mut / contains_key ===");
    println!("zip_map[\"A\"] = {}", zip_map["A"]);
    if let Some(v) = zip_map.get("B") {
        println!("get B = {}", v);
    }
    let copied = zip_map.get("A").copied().unwrap_or(0);
    println!("get A .copied().unwrap_or(0) = {}", copied);
    if let Some(v) = zip_map.get_mut("B") {
        *v += 5;
    }
    println!("after get_mut B+=5: {:?}", zip_map);
    println!("contains_key Red? {}", zip_map.contains_key("Red"));

    println!("\n=== 4) entry / or_insert 词频 ===");
    let mut count = HashMap::new();
    for s in ["a", "a", "b"] {
        *count.entry(s).or_insert(0) += 1;
    }
    println!("count = {:?}", count);

    count
        .entry("a")
        .and_modify(|v| *v *= 2)
        .or_insert(100);
    println!("and_modify a*=2: {:?}", count);

    println!("\n=== 5) 所有权 ===");
    let s = String::from("borrowed");
    let mut ref_map: HashMap<&str, i32> = HashMap::new();
    ref_map.insert(&s, 99);
    println!("insert &s 后 s 仍可用: {} map={:?}", s, ref_map);

    let mut own_map: HashMap<String, i32> = HashMap::new();
    let moved = String::from("moved");
    own_map.insert(moved, 1);
    // moved 已 move
    let num = 42;
    own_map.insert("num".to_string(), num);
    println!("i32 Copy 后 num 仍可用: {}", num);
    println!("own_map = {:?}", own_map);

    println!("\n=== 6) remove / remove_entry ===");
    let removed = zip_map.remove("A");
    let entry = zip_map.remove_entry("B");
    println!("remove A={:?}, remove_entry B={:?}, rest={:?}", removed, entry, zip_map);

    println!("\n=== 7) 遍历 keys / values / &map ===");
    let mut demo = HashMap::from([("x", 1), ("y", 2)]);
    print!("keys: ");
    for k in demo.keys() {
        print!("{} ", k);
    }
    println!();
    for (k, v) in demo.iter_mut() {
        *v += 10;
        println!("  {} -> {}", k, v);
    }

    println!("\nok: HashMap 创建/取值/Entry/所有权/删除/遍历");
}
