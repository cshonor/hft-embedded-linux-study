//! §6.5：教学版事件总线（std `HashMap` + `VecDeque`，无 Tokio）。

use std::collections::{HashMap, VecDeque};

#[derive(Clone, Debug)]
enum Event {
    Temp(i16),
}

struct EventBus {
    subs: HashMap<u64, VecDeque<Event>>,
    next_id: u64,
}

impl EventBus {
    fn new() -> Self {
        Self {
            subs: HashMap::new(),
            next_id: 1,
        }
    }
    fn subscribe(&mut self) -> u64 {
        let id = self.next_id;
        self.next_id += 1;
        self.subs.insert(id, VecDeque::new());
        id
    }
    fn publish(&mut self, ev: Event) {
        for q in self.subs.values_mut() {
            q.push_back(ev.clone());
        }
    }
}

fn main() {
    let mut bus = EventBus::new();
    let a = bus.subscribe();
    let b = bus.subscribe();
    bus.publish(Event::Temp(22));
    assert_eq!(bus.subs[&a].len(), 1);
    assert_eq!(bus.subs[&b].len(), 1);
    println!("§6.5 ok: two subscribers each got one event");

    // 顺带验证负载本身 —— 否则 Event::Temp 的字段从未被读取（dead_code 警告）。
    let q = bus.subs.get_mut(&a).expect("subscriber a");
    match q.pop_front().expect("event") {
        Event::Temp(t) => println!("§6.5 payload check: subscriber {a} received Temp({t})"),
    }
}
