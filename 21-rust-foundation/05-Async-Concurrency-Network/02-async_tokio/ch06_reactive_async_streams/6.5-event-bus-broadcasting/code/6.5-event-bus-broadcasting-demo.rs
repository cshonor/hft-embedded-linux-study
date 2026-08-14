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
}
