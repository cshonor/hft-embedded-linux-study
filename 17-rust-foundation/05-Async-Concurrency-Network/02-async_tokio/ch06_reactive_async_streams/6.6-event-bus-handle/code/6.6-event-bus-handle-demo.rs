//! §6.6：`EventHandle` drop 时 unsubscribe（教学 std 版）。

use std::collections::HashMap;

struct Bus {
    queues: HashMap<u64, Vec<u32>>,
}

struct Handle<'a> {
    id: u64,
    bus: &'a mut Bus,
}

impl Drop for Handle<'_> {
    fn drop(&mut self) {
        self.bus.queues.remove(&self.id);
        println!("unsubscribed id={}", self.id);
    }
}

fn main() {
    let mut bus = Bus {
        queues: HashMap::new(),
    };
    bus.queues.insert(1, vec![1, 2, 3]);
    {
        let _h = Handle { id: 1, bus: &mut bus };
    }
    assert!(!bus.queues.contains_key(&1));
    println!("§6.6 ok — no leak after handle drop");
}
