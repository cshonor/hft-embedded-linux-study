//! 4.3 切片 —— &str / &[T] + HFT 风格 &[u8] 零拷贝分段

fn main() {
    println!("=== 1. 字符串 slice ===");
    str_slices();

    println!("\n=== 2. first_word（&str）===");
    let s = String::from("hello world");
    println!("first word: '{}'", first_word(&s));

    println!("\n=== 3. 数组 slice ===");
    let a = [1, 2, 3, 4, 5];
    println!("&a[1..3] = {:?}", &a[1..3]);

    println!("\n=== 4. HFT：&[u8] 零拷贝报文分段 ===");
    hft_zero_copy_demo();

    println!("\n=== 5. Vec 缓冲区：切片绑定 vs 裸 usize ===");
    vec_packet_buffer_demo();

    println!("\n=== 6. 静态 &str vs 堆上 String 切片 ===");
    static_vs_heap_str_demo();
}

fn str_slices() {
    let s = String::from("hello world");
    println!("hello='{}', world='{}'", &s[0..5], &s[6..11]);

    let s = String::from("hello");
    println!(
        "[0..2]='{}', [..2]='{}', [3..]='{}', [..]='{}'",
        &s[0..2],
        &s[..2],
        &s[3..],
        &s[..]
    );
}

fn first_word(s: &str) -> &str {
    let bytes = s.as_bytes();
    for (i, &b) in bytes.iter().enumerate() {
        if b == b' ' {
            return &s[0..i];
        }
    }
    &s[..]
}

/// 模拟定长行情头：magic(2) + price_le(8) + qty_le(4) = 14 字节
/// 全程 `&[u8]` 子切片，不分配、不拷贝字段缓冲区。
fn hft_zero_copy_demo() {
    let mut buf = vec![0u8; 14];
    buf[0..2].copy_from_slice(b"TK");
    buf[2..10].copy_from_slice(&123_456_789_000_1234u64.to_le_bytes());
    buf[10..14].copy_from_slice(&500u32.to_le_bytes());

    // 整包借用；parse 只返回指向 buf 内部的切片视图
    let pkt: &[u8] = &buf;
    match parse_tick(pkt) {
        Some(tick) => {
            println!("magic = {:?}", tick.magic); // 仍是 &[u8]，零拷贝
            println!("price bytes = {:?}", tick.price_bytes);
            println!("qty bytes   = {:?}", tick.qty_bytes);
            println!(
                "price = {}, qty = {}",
                tick.price(),
                tick.qty()
            );
        }
        None => println!("packet too short"),
    }

    // 对比：若只存 usize 下标，buf.resize(0, 0) 后下标仍“存在”——逻辑 bug
    let bad_idx = 10usize;
    buf.clear();
    println!(
        "(反例) 裸下标 bad_idx={} 在 buf 清空后仍打印，但已无效",
        bad_idx
    );
}

/// `Vec<u8>` 模拟可复用的 UDP 接收缓冲区：切片绑定整段内存，而非裸下标。
fn vec_packet_buffer_demo() {
    let mut buf: Vec<u8> = vec![0; 20];
    buf[0..2].copy_from_slice(b"TK");
    buf[2..10].copy_from_slice(&99u64.to_le_bytes());
    buf[10..14].copy_from_slice(&1u32.to_le_bytes());

    // ✅ 推荐：&buf 得到 &[u8]，parse 返回的子切片都指向 buf 内部
    let pkt: &[u8] = &buf;
    if let Some(tick) = parse_tick(pkt) {
        println!("Vec 缓冲区内 magic = {:?}", tick.magic);
        println!("解码 price = {}", tick.price());
    }

    // 裸 usize：只记录「价格字段从偏移 2 开始」——与 buf 无生命周期绑定
    let price_offset: usize = 2;
    buf.truncate(0); // 模拟缓冲区被复用/清空
    println!(
        "裸下标 price_offset={} 仍在，但 buf.len()={}，用它切片会越界",
        price_offset,
        buf.len()
    );

    // 若写成下面这样且 tick 仍存活，则 buf.truncate(0) 无法编译（借用检查）：
    // let mut buf2 = vec![...]; let tick = parse_tick(&buf2).unwrap(); buf2.clear();
}

/// 来源 1：字面量 → 指向只读静态区（`'static`，与程序同寿）
/// 来源 2：`String` 切片 → 借用堆内存，不能比 `String` 活得更久
fn static_vs_heap_str_demo() {
    let lit: &'static str = "literal-static";
    println!("'static: {lit}");

    let st = String::from("from-heap");
    let part: &str = &st[..];
    println!("from String slice: {part}");
    // `st` 在本作用域结束前有效；`part` 不能逃出此作用域（见 compile_fail/part_outlives_string.rs）
}

#[derive(Debug, Clone, Copy)]
struct TickView<'a> {
    magic: &'a [u8],
    price_bytes: &'a [u8],
    qty_bytes: &'a [u8],
}

fn parse_tick(pkt: &[u8]) -> Option<TickView<'_>> {
    const HEADER: usize = 14;
    if pkt.len() < HEADER {
        return None;
    }
    Some(TickView {
        magic: &pkt[0..2],
        price_bytes: &pkt[2..10],
        qty_bytes: &pkt[10..14],
    })
}

impl TickView<'_> {
    fn price(&self) -> u64 {
        let mut b = [0u8; 8];
        b.copy_from_slice(self.price_bytes);
        u64::from_le_bytes(b)
    }

    fn qty(&self) -> u32 {
        let mut b = [0u8; 4];
        b.copy_from_slice(self.qty_bytes);
        u32::from_le_bytes(b)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_tick_ok() {
        let mut buf = vec![0u8; 14];
        buf[0..2].copy_from_slice(b"TK");
        buf[2..10].copy_from_slice(&42u64.to_le_bytes());
        buf[10..14].copy_from_slice(&7u32.to_le_bytes());

        let v = parse_tick(&buf).unwrap();
        assert_eq!(v.magic, b"TK");
        assert_eq!(v.price(), 42);
        assert_eq!(v.qty(), 7);
    }

    #[test]
    fn parse_tick_short_packet() {
        assert!(parse_tick(&[1, 2, 3]).is_none());
    }

    #[test]
    fn first_word_slice() {
        assert_eq!(first_word("abc def"), "abc");
    }

    #[test]
    fn vec_slice_views_into_same_buffer() {
        let mut buf = vec![0u8; 14];
        buf[0..2].copy_from_slice(b"TK");
        buf[2..10].copy_from_slice(&1u64.to_le_bytes());
        buf[10..14].copy_from_slice(&2u32.to_le_bytes());

        let tick = parse_tick(&buf).unwrap();
        assert_eq!(tick.magic, b"TK");
        assert_eq!(tick.price(), 1);
    }

    /// 模拟官方 4.3：`idx` 仍是 5 但 `s` 已空 —— 编译器不在 clear 时报错，切片时才 panic
    #[test]
    #[should_panic]
    fn dangling_usize_panics_at_runtime() {
        let mut s = String::from("hello world");
        let idx = 5usize; // 空格下标；与 s 无借用关系
        s.clear();
        let _ = &s[0..idx]; // 运行时越界
    }
}
