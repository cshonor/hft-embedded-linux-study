//! lob_test.rs — 对照 part-a-lob/lob_test.cpp 的五个用例 + Phase 2 用例
//!
//! 前五个测试与 C++ 版**逐断言对齐**（同名、同数据、同预期）。

use p8_lob::{Book, Order, Side};

// ---------- Phase 1：与 part-a-lob/lob_test.cpp 逐断言对齐 ----------

#[test]
fn best_price() {
    // C++: submit(sell 10400×50), submit(sell 10300×200), buy 10500×100 → 10300 成交
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10400, 50));
    b.submit(Order::limit(2, Side::Sell, 10300, 200));
    let t = b.submit(Order::limit(3, Side::Buy, 10500, 100));
    assert_eq!(t.len(), 1);
    assert_eq!(t[0].price, 10300);
    assert_eq!(t[0].qty, 100);
    assert_eq!(t[0].maker_id, 2);
    assert_eq!(t[0].taker_id, 3);
}

#[test]
fn fifo() {
    // C++: 两个同价 sell，buy 先吃 id=1
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 10));
    b.submit(Order::limit(2, Side::Sell, 10000, 10));
    let t = b.submit(Order::limit(3, Side::Buy, 10000, 10));
    assert_eq!(t.len(), 1);
    assert_eq!(t[0].maker_id, 1);
}

#[test]
fn no_match() {
    // C++: 不成交则双方挂簿，best 正确
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 9900, 10));
    let t = b.submit(Order::limit(2, Side::Buy, 9800, 10));
    assert!(t.is_empty());
    assert_eq!(b.best_bid(), Some(9800));
    assert_eq!(b.best_ask(), Some(9900));
}

#[test]
fn partial() {
    // C++: 部分成交，剩余仍挂
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 50));
    let t = b.submit(Order::limit(2, Side::Buy, 10000, 20));
    assert_eq!(t.len(), 1);
    assert_eq!(t[0].qty, 20);
    assert_eq!(b.best_ask(), Some(10000));
}

#[test]
fn cancel() {
    // C++: 撤掉 id=1 后 FIFO 递补到 id=2
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 10));
    b.submit(Order::limit(2, Side::Sell, 10000, 10));
    assert!(b.cancel(1));
    let t = b.submit(Order::limit(3, Side::Buy, 10000, 10));
    assert_eq!(t.len(), 1);
    assert_eq!(t[0].maker_id, 2);
}

// ---------- Phase 2：Market / IOC / FOK ----------

#[test]
fn market_eats_book() {
    // 市价买：不检查价格，连穿两个价位直到数量吃净
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 10));
    b.submit(Order::limit(2, Side::Sell, 10100, 10));
    let t = b.submit(Order::market(3, Side::Buy, 15));
    assert_eq!(t.len(), 2);
    assert_eq!(t[0].price, 10000);
    assert_eq!(t[0].qty, 10);
    assert_eq!(t[1].price, 10100);
    assert_eq!(t[1].qty, 5);
    // 剩余 5 还挂在 10100
    assert_eq!(b.best_ask(), Some(10100));
}

#[test]
fn market_empty_book_no_rest() {
    // 市价单剩余不挂簿
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 5));
    let t = b.submit(Order::market(2, Side::Buy, 50));
    assert_eq!(t.len(), 1);
    assert_eq!(t[0].qty, 5);
    assert_eq!(b.best_bid(), None); // 剩余 45 不挂
}

#[test]
fn ioc_cancels_rest() {
    // IOC：限价撮合，剩余立即撤销（不挂簿）
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 5));
    let t = b.submit(Order::ioc(2, Side::Buy, 10000, 50));
    assert_eq!(t.len(), 1);
    assert_eq!(t[0].qty, 5);
    assert_eq!(b.best_bid(), None); // 剩余 45 已撤销
}

#[test]
fn ioc_no_cross_nothing_happens() {
    // IOC 价格不够：一笔不成交，也不挂簿
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 5));
    let t = b.submit(Order::ioc(2, Side::Buy, 9900, 50));
    assert!(t.is_empty());
    assert_eq!(b.best_bid(), None);
    assert_eq!(b.best_ask(), Some(10000));
}

#[test]
fn fok_fillable_executes_all() {
    // FOK 能全部成交：正常执行（两个价位凑齐 15）
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 10));
    b.submit(Order::limit(2, Side::Sell, 10050, 10));
    let t = b.submit(Order::fok(3, Side::Buy, 10050, 15));
    assert_eq!(t.len(), 2);
    assert_eq!(t[0].qty + t[1].qty, 15);
}

#[test]
fn fok_unfillable_kills_all() {
    // FOK 不够量：全部撤销，对手簿原封不动
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Sell, 10000, 10));
    let t = b.submit(Order::fok(2, Side::Buy, 10000, 50));
    assert!(t.is_empty());
    // 对手簿没被吃掉（若部分成交就不符合 FOK 语义）
    assert_eq!(b.best_ask(), Some(10000));
    // 再买一个 10 验证簿子完好
    let t2 = b.submit(Order::limit(3, Side::Buy, 10000, 10));
    assert_eq!(t2.len(), 1);
    assert_eq!(t2[0].qty, 10);
}

#[test]
fn sell_side_symmetry() {
    // 卖单侧对称：sell 吃 bids，最优买先成交
    let mut b = Book::new();
    b.submit(Order::limit(1, Side::Buy, 9900, 10));
    b.submit(Order::limit(2, Side::Buy, 10000, 10));
    let t = b.submit(Order::limit(3, Side::Sell, 9800, 15));
    assert_eq!(t.len(), 2);
    assert_eq!(t[0].price, 10000); // 先吃最优买
    assert_eq!(t[0].maker_id, 2);
    assert_eq!(t[1].price, 9900);
}
