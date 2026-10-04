/*
 * test_protocol.cpp — Phase 1 自测：线尺寸、编解码回环、畸形报文、序列号状态机。
 *
 * 不引 gtest：这个工程要在裸 Linux/Pi 上随手可跑，断言宏 30 行够用。
 * 跑法：cmake --build build && ./build/test_protocol
 */

#include <cstdio>
#include <cstring>
#include <cstdint>

#include "protocol/codec.hpp"
#include "protocol/sequencer.hpp"
#include "common/time.hpp"

using namespace hft;
using namespace hft::proto;

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

// ---- 1. 线尺寸锁定（协议改动的防火墙）----
static void test_wire_sizes() {
    static_assert(kFrameHeaderSize == 3);
    static_assert(kHeaderWireSize == 16);
    static_assert(message_wire_size(MsgType::Heartbeat)    == 19);
    static_assert(message_wire_size(MsgType::NewOrder)     == 48);
    static_assert(message_wire_size(MsgType::CancelOrder)  == 27);
    static_assert(message_wire_size(MsgType::ReplaceOrder) == 43);
    static_assert(message_wire_size(MsgType::Trade)        == 51);
    static_assert(message_wire_size(MsgType::GapFill)      == 35);
    // Message 不能引入堆/指针：可平凡拷贝，memcpy 安全。
    static_assert(__is_trivially_copyable(Message));
    g_pass += 1; // static_assert 全部通过即达标
}

// ---- 2. 逐类型编码→解码回环 ----
template <typename Fill, typename Verify>
static void roundtrip(MsgType t, Fill fill, Verify verify) {
    Message tx{};
    tx.type    = t;
    tx.hdr.seq = 42;
    tx.hdr.ts  = 1'234'567'890'123ull;
    fill(tx);

    std::uint8_t buf[128];
    const std::size_t n = encode(buf, sizeof(buf), tx);
    CHECK(n == message_wire_size(t));

    Message rx{};
    const std::size_t used = decode(buf, n, rx);
    CHECK(used == n);
    CHECK(rx.type == t);
    CHECK(rx.hdr.seq == 42);
    CHECK(rx.hdr.ts == 1'234'567'890'123ull);
    verify(rx);
}

static void test_roundtrips() {
    roundtrip(MsgType::Heartbeat, [](Message&) {}, [](Message&) {});
    roundtrip(MsgType::SessionEnd, [](Message&) {}, [](Message&) {});
    roundtrip(MsgType::SessionStart,
        [](Message& m) { m.session_start.symbol = 600519; },
        [](Message& m) { CHECK(m.session_start.symbol == 600519); });
    roundtrip(MsgType::NewOrder,
        [](Message& m) {
            m.new_order.id     = 0xDEADBEEFCAFEull;
            m.new_order.symbol = 7;
            m.new_order.price  = -12345; // 负价也要能过（期货套利腿）
            m.new_order.qty    = 100;
            m.new_order.side   = Side::Sell;
        },
        [](Message& m) {
            CHECK(m.new_order.id == 0xDEADBEEFCAFEull);
            CHECK(m.new_order.symbol == 7);
            CHECK(m.new_order.price == -12345);
            CHECK(m.new_order.qty == 100);
            CHECK(m.new_order.side == Side::Sell);
        });
    roundtrip(MsgType::CancelOrder,
        [](Message& m) { m.cancel_order.id = 99; },
        [](Message& m) { CHECK(m.cancel_order.id == 99); });
    roundtrip(MsgType::ReplaceOrder,
        [](Message& m) {
            m.replace_order.id        = 5;
            m.replace_order.new_price = 8888;
            m.replace_order.new_qty   = 66;
        },
        [](Message& m) {
            CHECK(m.replace_order.id == 5);
            CHECK(m.replace_order.new_price == 8888);
            CHECK(m.replace_order.new_qty == 66);
        });
    roundtrip(MsgType::Trade,
        [](Message& m) {
            m.trade.aggressor_id = 11;
            m.trade.resting_id   = 22;
            m.trade.price        = 31400;
            m.trade.qty          = 50;
        },
        [](Message& m) {
            CHECK(m.trade.aggressor_id == 11);
            CHECK(m.trade.resting_id == 22);
            CHECK(m.trade.price == 31400);
            CHECK(m.trade.qty == 50);
        });
    roundtrip(MsgType::GapFill,
        [](Message& m) { m.gap_fill.from = 100; m.gap_fill.to = 120; },
        [](Message& m) { CHECK(m.gap_fill.from == 100); CHECK(m.gap_fill.to == 120); });
}

// ---- 3. 大端字节序显式校验（防"换台机器悄悄错"）----
static void test_endianness_explicit() {
    Message tx{};
    tx.type    = MsgType::CancelOrder;
    tx.hdr.seq = 1;
    tx.hdr.ts  = 0;
    tx.cancel_order.id = 0x0102030405060708ull;

    std::uint8_t buf[64];
    const std::size_t n = encode(buf, sizeof(buf), tx);
    CHECK(n == message_wire_size(MsgType::CancelOrder));
    // 帧头: 00 1B('27') 'X'；然后 seq(8B) ts(8B)；id 的最后 8 字节必须升序 01..08。
    CHECK(buf[0] == 0x00 && buf[1] == 0x1B && buf[2] == 'X');
    for (int i = 0; i < 8; ++i) {
        CHECK(buf[n - 8 + i] == static_cast<std::uint8_t>(i + 1));
    }
}

// ---- 4. 畸形/截断报文必须 reject，不能 UB ----
static void test_malformed() {
    std::uint8_t buf[128];
    Message tx{};
    tx.type    = MsgType::NewOrder;
    tx.hdr.seq = 1;
    tx.new_order.id = 1; tx.new_order.symbol = 1;
    tx.new_order.price = 100; tx.new_order.qty = 1;
    tx.new_order.side = Side::Buy;
    const std::size_t n = encode(buf, sizeof(buf), tx);
    CHECK(n > 0);

    Message rx{};
    // 截断：少 1 字节。
    CHECK(decode(buf, n - 1, rx) == 0);
    // 长度字段造假（声称比实际长）。
    std::uint8_t evil[128];
    std::memcpy(evil, buf, n);
    evil[0] = 0xFF; evil[1] = 0xFF;
    CHECK(decode(evil, n, rx) == 0);
    // 长度字段与协议表不符。
    std::memcpy(evil, buf, n);
    evil[1] = static_cast<std::uint8_t>(n - 2); // total 改小
    CHECK(decode(evil, n, rx) == 0);
    // Side 越界。
    std::memcpy(evil, buf, n);
    evil[n - 1] = 7;
    CHECK(decode(evil, n, rx) == 0);
    // 空报文 / 超短报文。
    CHECK(decode(buf, 0, rx) == 0);
    CHECK(decode(buf, 2, rx) == 0);
    // 编码缓冲不足。
    std::uint8_t tiny[8];
    CHECK(encode(tiny, sizeof(tiny), tx) == 0);
}

// ---- 5. 一个 UDP 报文装多条消息，游标连续解码 ----
static void test_multi_message_datagram() {
    std::uint8_t buf[256];
    std::size_t off = 0;

    Message a{};
    a.type = MsgType::Heartbeat; a.hdr.seq = 1;
    off += encode(buf + off, sizeof(buf) - off, a);

    Message b{};
    b.type = MsgType::CancelOrder; b.hdr.seq = 2; b.cancel_order.id = 777;
    off += encode(buf + off, sizeof(buf) - off, b);

    Message c{};
    c.type = MsgType::GapFill; c.hdr.seq = 5; c.gap_fill.from = 3; c.gap_fill.to = 4;
    off += encode(buf + off, sizeof(buf) - off, c);

    std::size_t pos = 0;
    Message m{};
    pos += decode(buf + pos, off - pos, m);
    CHECK(m.type == MsgType::Heartbeat && m.hdr.seq == 1);
    pos += decode(buf + pos, off - pos, m);
    CHECK(m.type == MsgType::CancelOrder && m.cancel_order.id == 777);
    pos += decode(buf + pos, off - pos, m);
    CHECK(m.type == MsgType::GapFill && m.gap_fill.from == 3 && m.gap_fill.to == 4);
    CHECK(pos == off);
}

// ---- 6. 序列号状态机 ----
static void test_sequencer() {
    Sequencer s;
    s.reset(1);
    CHECK(s.on_message(1) == SeqVerdict::Accept);
    CHECK(s.on_message(2) == SeqVerdict::Accept);
    CHECK(s.on_message(2) == SeqVerdict::Duplicate); // 重传包
    CHECK(s.on_message(1) == SeqVerdict::Duplicate);
    CHECK(s.on_message(5) == SeqVerdict::Gap);       // 丢了 3,4
    CHECK(s.expect() == 3);                          // gap 不推进 expect

    // 乱序的 3 补到（重传恢复后按序交付）：先 resync 到 5。
    s.resync(5);
    CHECK(s.on_message(5) == SeqVerdict::Accept);

    // GapFill：必须从 expect 开始且区间合法。
    CHECK(s.expect() == 6);
    CHECK(s.on_gap_fill(7, 8) == false);  // 起点错
    CHECK(s.on_gap_fill(6, 5) == false);  // 区间反转
    CHECK(s.on_gap_fill(6, 9) == true);
    CHECK(s.expect() == 10);
}

// ---- 7. 计时器冒烟：单调性 ----
static void test_clock_smoke() {
    const std::uint64_t a = now_ns();
    const std::uint64_t b = now_ns();
    CHECK(b >= a);
    const std::uint64_t c0 = rdcycle();
    const std::uint64_t c1 = rdcycle();
    CHECK(c1 >= c0);
}

int main() {
    test_wire_sizes();
    test_roundtrips();
    test_endianness_explicit();
    test_malformed();
    test_multi_message_datagram();
    test_sequencer();
    test_clock_smoke();

    std::printf("protocol self-test: pass=%d fail=%d\n", g_pass, g_fail);
    if (g_fail == 0) {
        std::puts("ALL OK");
        return 0;
    }
    return 1;
}
