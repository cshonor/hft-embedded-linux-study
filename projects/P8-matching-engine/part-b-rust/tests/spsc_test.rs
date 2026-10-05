//! spsc_test.rs — SPSC ring + 行情→撮合链路测试（P8 Phase 3 验收）
//!
//! 对照 P8 README Phase 3 的验收：「生产者猛推 100 万订单，消费者撮合，验证无丢无重」。

use p8_lob::{Book, Order, Side, SpscRing};
use std::sync::Arc;
use std::thread;

#[test]
fn fifo_order_single_thread() {
    // 单线程：push 顺序 == pop 顺序（FIFO）
    let r = SpscRing::<u64, 8>::new();
    for i in 0..8u64 {
        r.try_push(i).unwrap();
    }
    assert!(r.try_push(99).is_err()); // 满了：背压
    for i in 0..8u64 {
        assert_eq!(r.try_pop(), Some(i));
    }
    assert_eq!(r.try_pop(), None); // 空了
}

#[test]
fn wraparound_keeps_order() {
    // 绕圈：反复 push/pop 跨数组边界，顺序不乱
    let r = SpscRing::<u64, 4>::new();
    for i in 0..100u64 {
        while r.try_push(i).is_err() {}
        assert_eq!(r.try_pop(), Some(i));
    }
}

#[test]
fn full_returns_err_with_value_back() {
    // 满时 try_push 把原值还回来（调用方自旋用）
    let r = SpscRing::<u64, 2>::new();
    r.try_push(1).unwrap();
    r.try_push(2).unwrap();
    assert_eq!(r.try_push(3), Err(3));
    assert_eq!(r.try_pop(), Some(1));
    r.try_push(3).unwrap();
    assert_eq!(r.try_pop(), Some(2));
    assert_eq!(r.try_pop(), Some(3));
}

#[test]
fn million_orders_no_loss_no_dup() {
    // P8 README 验收原版：生产者猛推 100 万订单，消费者撮合，验证无丢无重
    const TOTAL: u64 = 1_000_000;
    let ring = Arc::new(SpscRing::<Order, 4096>::new());

    // 生产者（模拟行情输入线程）：id 递增的确定性订单流
    let producer = {
        let ring = Arc::clone(&ring);
        thread::spawn(move || {
            for i in 1..=TOTAL {
                let side = if i % 2 == 0 { Side::Buy } else { Side::Sell };
                // 价格围绕 10000 小幅波动，保证大量可撮合
                let price = 10000 + (i % 7) as i64 - 3;
                let order = Order::limit(i, side, price, (i % 10 + 1) as i64);
                while ring.try_push(order).is_err() {
                    std::hint::spin_loop(); // 背压：自旋，不睡觉
                }
            }
        })
    };

    // 消费者（撮合线程）：pop → Book.submit → 统计
    let consumer = {
        let ring = Arc::clone(&ring);
        thread::spawn(move || {
            let mut book = Book::new();
            let mut popped = 0u64;
            let mut trades = 0usize;
            let mut last_id = 0u64;
            while popped < TOTAL {
                match ring.try_pop() {
                    Some(o) => {
                        assert!(o.id > last_id, "乱序/重复: id {} after {}", o.id, last_id);
                        last_id = o.id;
                        trades += book.submit(o).len();
                        popped += 1;
                    }
                    None => std::hint::spin_loop(),
                }
            }
            (popped, trades, last_id)
        })
    };

    producer.join().unwrap();
    let (popped, trades, last_id) = consumer.join().unwrap();

    assert_eq!(popped, TOTAL, "无丢：pop 数 == push 数");
    assert_eq!(last_id, TOTAL, "无重：最后一个 id == TOTAL（若重复 last_id 会超）");
    assert!(trades > 0, "订单流应该产生大量撮合（价格带 ±3 穿档）");
    println!("1M orders OK, {} trades", trades);
}

#[test]
fn consumer_result_matches_single_thread_book() {
    // 同一份确定性订单流：ring 传递后撮合的结果 == 单线程直接 submit 的结果
    let stream: Vec<Order> = (1..=10_000u64)
        .map(|i| {
            let side = if i % 3 == 0 { Side::Sell } else { Side::Buy };
            Order::limit(i, side, 9900 + (i * 37 % 201) as i64, (i % 5 + 1) as i64)
        })
        .collect();

    // 基准：单线程
    let mut baseline = Book::new();
    let mut baseline_trades = 0usize;
    for &o in &stream {
        baseline_trades += baseline.submit(o).len();
    }

    // 经由 ring 的生产者-消费者
    let ring = Arc::new(SpscRing::<Order, 1024>::new());
    let producer = {
        let ring = Arc::clone(&ring);
        let stream = stream.clone();
        thread::spawn(move || {
            for o in stream {
                while ring.try_push(o).is_err() {
                    std::hint::spin_loop();
                }
            }
        })
    };
    let consumer = {
        let ring = Arc::clone(&ring);
        let total = stream.len();
        thread::spawn(move || {
            let mut book = Book::new();
            let mut trades = 0usize;
            for _ in 0..total {
                loop {
                    if let Some(o) = ring.try_pop() {
                        trades += book.submit(o).len();
                        break;
                    }
                    std::hint::spin_loop();
                }
            }
            (book, trades)
        })
    };
    producer.join().unwrap();
    let (book, trades) = consumer.join().unwrap();

    assert_eq!(trades, baseline_trades, "经由 ring 的成交数必须等于单线程基准");
    assert_eq!(book.best_bid(), baseline.best_bid());
    assert_eq!(book.best_ask(), baseline.best_ask());
}
