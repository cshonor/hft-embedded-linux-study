/*
 * test_book.cpp — Phase 2 自测：内存池、价位位图、订单簿撮合语义。
 *
 * 覆盖：BBO 维护、FIFO 优先级、扫多档、市价 IOC、撤单、改单排队规则、
 *       参数校验、池耗尽、最优价重扫（含跨侧情形）。
 */

#include <cstdio>

#include "book/order_book.hpp"

using namespace hft;

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (cond) { ++g_pass; }                                         \
        else {                                                          \
            ++g_fail;                                                   \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                               \
    } while (0)

// 成交捕获器：记录逐笔 (aggressor, resting, price, qty)。
struct TradeLog {
    proto::Trade t[64];
    int n = 0;
    void operator()(const proto::Trade& tr) { if (n < 64) t[n++] = tr; }
    void reset() { n = 0; }
};

using Book = OrderBook<1024, 4096>; // 窗口 [base, base+4095]
static constexpr Price BASE = 10000; // 价格 100.00 为窗口底

// ---- 1. 内存池 ----
static void test_pool() {
    Pool<Order> p(4);
    CHECK(p.available() == 4);
    Order* a = p.alloc(); CHECK(a != nullptr && p.available() == 3);
    Order* b = p.alloc(); Order* c = p.alloc(); Order* d = p.alloc();
    CHECK(p.available() == 0);
    CHECK(p.alloc() == nullptr); // 耗尽：nullptr，不抛不扩
    p.free(b);
    CHECK(p.available() == 1);
    CHECK(p.alloc() == b);       // LIFO 复用
    (void)a; (void)c; (void)d;
}

// ---- 2. 价位位图 ----
static void test_bitmap() {
    LevelBitmap<4096> bm;
    CHECK(bm.find_first() == -1 && bm.find_last() == -1);
    bm.set(5); bm.set(64); bm.set(100); bm.set(4095);
    CHECK(bm.find_first() == 5);
    CHECK(bm.find_last() == 4095);
    bm.clear(4095);
    CHECK(bm.find_last() == 100);
    // 区间查找
    CHECK(bm.find_first_above(5) == 64);
    CHECK(bm.find_first_above(63) == 64);
    CHECK(bm.find_last_below(64) == 5);
    CHECK(bm.find_last_below(100) == 64);
    CHECK(bm.find_first_above(4094) == -1);
    CHECK(bm.find_last_below(0) == -1);
    bm.clear(5); bm.clear(64); bm.clear(100);
    CHECK(bm.find_first() == -1 && bm.find_last() == -1);
}

// ---- 3. 挂单与 BBO ----
static void test_resting_bbo() {
    Book b(BASE);
    TradeLog log;
    CHECK(b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log) == BookError::Ok);
    CHECK(b.submit(2, Side::Buy, BASE + 101, 5, OrderType::Limit, log) == BookError::Ok);
    CHECK(b.submit(3, Side::Sell, BASE + 105, 10, OrderType::Limit, log) == BookError::Ok);
    CHECK(b.submit(4, Side::Sell, BASE + 104, 3, OrderType::Limit, log) == BookError::Ok);
    CHECK(log.n == 0); // 不交叉：无成交
    CHECK(b.has_bid() && b.has_ask());
    CHECK(b.best_bid() == BASE + 101);
    CHECK(b.best_ask() == BASE + 104);
    CHECK(b.best_bid_qty() == 5);
    CHECK(b.best_ask_qty() == 3);
    CHECK(b.resting() == 4);
    // 深度
    CHECK(b.level_qty(BASE + 100) == 10);
    CHECK(b.level_qty(BASE + 999) == 0);
}

// ---- 4. FIFO：同价位先挂先成交 ----
static void test_fifo_priority() {
    Book b(BASE);
    TradeLog log;
    b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(2, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(3, Side::Sell, BASE + 100, 15, OrderType::Limit, log);
    CHECK(log.n == 2);
    CHECK(log.t[0].resting_id == 1 && log.t[0].qty == 10);
    CHECK(log.t[1].resting_id == 2 && log.t[1].qty == 5);
    CHECK(log.t[0].price == BASE + 100); // 成交价 = 挂单方价
    CHECK(log.t[0].aggressor_id == 3);
    CHECK(b.resting() == 1);             // 单 2 剩 5
    CHECK(b.best_bid_qty() == 5);
}

// ---- 5. 扫多档：价格优先于时间 ----
static void test_sweep_levels() {
    Book b(BASE);
    TradeLog log;
    b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(2, Side::Buy, BASE + 101, 5, OrderType::Limit, log);
    b.submit(3, Side::Sell, BASE + 99, 25, OrderType::Limit, log);
    CHECK(log.n == 2);
    CHECK(log.t[0].price == BASE + 101 && log.t[0].resting_id == 2); // 先吃高价
    CHECK(log.t[1].price == BASE + 100 && log.t[1].resting_id == 1);
    CHECK(b.resting() == 1);              // 卖单剩 10 驻留在 99
    CHECK(!b.has_bid());
    CHECK(b.best_ask() == BASE + 99 && b.best_ask_qty() == 10);
}

// ---- 6. 部分成交后余量驻留 ----
static void test_partial_rest() {
    Book b(BASE);
    TradeLog log;
    b.submit(1, Side::Sell, BASE + 105, 5, OrderType::Limit, log);
    b.submit(2, Side::Buy, BASE + 105, 20, OrderType::Limit, log);
    CHECK(log.n == 1 && log.t[0].qty == 5);
    CHECK(b.resting() == 1);
    CHECK(b.best_bid() == BASE + 105 && b.best_bid_qty() == 15);
    CHECK(!b.has_ask()); // 卖单被吃光，最优卖价无效化
}

// ---- 7. 市价单：扫到没量为止，余量不驻留 ----
static void test_market_order() {
    Book b(BASE);
    TradeLog log;
    // 空簿市价：无成交无驻留
    CHECK(b.submit(1, Side::Buy, 0, 100, OrderType::Market, log) == BookError::Ok);
    CHECK(log.n == 0 && b.resting() == 0);

    b.submit(2, Side::Sell, BASE + 105, 10, OrderType::Limit, log);
    b.submit(3, Side::Sell, BASE + 107, 10, OrderType::Limit, log);
    log.reset();
    b.submit(4, Side::Buy, 0, 25, OrderType::Market, log);
    CHECK(log.n == 2);
    CHECK(log.t[0].price == BASE + 105 && log.t[0].qty == 10);
    CHECK(log.t[1].price == BASE + 107 && log.t[1].qty == 10);
    CHECK(b.resting() == 0); // 余 5 量 IOC 取消，不驻留
}

// ---- 8. 撤单 ----
static void test_cancel() {
    Book b(BASE);
    TradeLog log;
    CHECK(b.cancel(42) == BookError::UnknownId);
    b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(2, Side::Buy, BASE + 101, 5, OrderType::Limit, log);
    CHECK(b.best_bid() == BASE + 101);
    CHECK(b.cancel(2) == BookError::Ok);
    CHECK(b.best_bid() == BASE + 100); // 最优价重扫回落
    CHECK(b.cancel(2) == BookError::UnknownId); // 重复撤
    CHECK(b.resting() == 1);
    b.cancel(1);
    CHECK(!b.has_bid());
    CHECK(b.pool_available() == 1024); // 全部归还池
}

// ---- 9. 改单：同价减量保留排队位置 ----
static void test_replace_reduce_keeps_position() {
    Book b(BASE);
    TradeLog log;
    b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(2, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    CHECK(b.replace(1, BASE + 100, 8, log) == BookError::Ok); // 减量保位
    CHECK(b.level_qty(BASE + 100) == 18);
    log.reset();
    b.submit(3, Side::Sell, BASE + 100, 20, OrderType::Limit, log);
    CHECK(log.n == 2);
    CHECK(log.t[0].resting_id == 1 && log.t[0].qty == 8);  // 1 仍排最前
    CHECK(log.t[1].resting_id == 2 && log.t[1].qty == 10);
    CHECK(b.resting() == 1); // 单 2 已清，卖单剩 2 驻留
}

// ---- 10. 改价 = 丢位置（撤旧挂新） ----
static void test_replace_price_loses_position() {
    Book b(BASE);
    TradeLog log;
    b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(2, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    CHECK(b.replace(1, BASE + 101, 10, log) == BookError::Ok); // 改价
    CHECK(b.best_bid() == BASE + 101);
    CHECK(b.level_qty(BASE + 100) == 10);
    // 改到新价后若与卖单交叉要成交
    b.submit(3, Side::Sell, BASE + 102, 5, OrderType::Limit, log);
    log.reset();
    CHECK(b.replace(1, BASE + 102, 10, log) == BookError::Ok);
    CHECK(log.n == 1 && log.t[0].qty == 5 && log.t[0].price == BASE + 102);
    CHECK(b.resting() == 2); // 单 2 @100，单 1 余 5 @102
    // 加量也丢位置
    CHECK(b.replace(2, BASE + 100, 99, log) == BookError::Ok);
}

// ---- 11. 参数校验 ----
static void test_validation() {
    Book b(BASE);
    TradeLog log;
    CHECK(b.submit(1, Side::Buy, BASE + 100, 0, OrderType::Limit, log) == BookError::BadQty);
    CHECK(b.submit(1, Side::Buy, BASE + 100, -5, OrderType::Limit, log) == BookError::BadQty);
    CHECK(b.submit(1, Side::Buy, BASE - 1, 10, OrderType::Limit, log) == BookError::BadPrice);
    CHECK(b.submit(1, Side::Buy, BASE + 4096, 10, OrderType::Limit, log) == BookError::BadPrice);
    CHECK(b.submit(0, Side::Buy, BASE + 100, 10, OrderType::Limit, log) == BookError::DuplicateId);
    CHECK(b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log) == BookError::Ok);
    CHECK(b.submit(1, Side::Buy, BASE + 101, 10, OrderType::Limit, log) == BookError::DuplicateId);
    CHECK(b.replace(99, BASE + 100, 5, log) == BookError::UnknownId);
    CHECK(b.replace(1, BASE + 100, -1, log) == BookError::BadQty);
    CHECK(b.replace(1, BASE - 5, 5, log) == BookError::BadPrice);
}

// ---- 12. 池耗尽：拒单不崩 ----
static void test_pool_exhaustion() {
    OrderBook<8, 64> b(0);
    TradeLog log;
    for (OrderId i = 1; i <= 8; ++i) {
        CHECK(b.submit(i, Side::Buy, static_cast<Price>(i), 1, OrderType::Limit, log) == BookError::Ok);
    }
    CHECK(b.submit(9, Side::Buy, 9, 1, OrderType::Limit, log) == BookError::PoolExhausted);
    CHECK(b.resting() == 8);
    // 撤一单后又能挂
    b.cancel(3);
    CHECK(b.submit(9, Side::Buy, 9, 1, OrderType::Limit, log) == BookError::Ok);
}

// ---- 13. 最优价重扫：跨侧情形（买一清空后最高非空是卖价） ----
static void test_rescan_across_sides() {
    Book b(BASE);
    TradeLog log;
    b.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log);
    b.submit(2, Side::Sell, BASE + 110, 10, OrderType::Limit, log);
    b.cancel(1);
    // 最高非空价位是 110（卖侧），买盘必须正确判定为空
    CHECK(!b.has_bid());
    CHECK(b.has_ask() && b.best_ask() == BASE + 110);
    // 反过来：卖一清空后最低非空是买价
    Book b2(BASE);
    TradeLog log2;
    b2.submit(1, Side::Buy, BASE + 100, 10, OrderType::Limit, log2);
    b2.submit(2, Side::Sell, BASE + 110, 10, OrderType::Limit, log2);
    b2.cancel(2);
    CHECK(!b2.has_ask());
    CHECK(b2.has_bid() && b2.best_bid() == BASE + 100);
}

int main() {
    test_pool();
    test_bitmap();
    test_resting_bbo();
    test_fifo_priority();
    test_sweep_levels();
    test_partial_rest();
    test_market_order();
    test_cancel();
    test_replace_reduce_keeps_position();
    test_replace_price_loses_position();
    test_validation();
    test_pool_exhaustion();
    test_rescan_across_sides();

    std::printf("book self-test: pass=%d fail=%d\n", g_pass, g_fail);
    if (g_fail == 0) {
        std::puts("ALL OK");
        return 0;
    }
    return 1;
}
