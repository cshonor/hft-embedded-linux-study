//! 借用检查器原生支持：同一 struct 不同字段的可变借用不重叠。

pub struct Pair {
    pub a: u32,
    pub b: u32,
}

pub fn split_fields(pair: &mut Pair) -> (&mut u32, &mut u32) {
    (&mut pair.a, &mut pair.b)
}

pub fn demo() -> (u32, u32) {
    let mut p = Pair { a: 10, b: 20 };
    let (a, b) = split_fields(&mut p);
    *a += 1;
    *b += 2;
    (p.a, p.b)
}
