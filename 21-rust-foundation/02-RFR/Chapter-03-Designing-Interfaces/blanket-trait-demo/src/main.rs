//! ```bash
//! cargo run --manifest-path 02-RFR/Chapter-03-Designing-Interfaces/blanket-trait-demo/Cargo.toml
//! ```

use rfr_blanket_trait_demo::{Count, Foo, Items, MyTrait};
use std::rc::Rc;
use std::sync::Arc;

fn main() {
    let mut f = Foo;
    f.work();
    (&f).work();
    (&mut f).work();
    Box::new(Foo).work();
    Arc::new(Foo).work();
    Rc::new(Foo).work();

    let items = Items(vec![1, 2, 3]);
    assert_eq!((&items).len(), 3);
    assert!(!(&items).is_empty());

    println!("blanket trait demo ok");
}
