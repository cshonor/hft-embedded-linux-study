/* test_feed.cpp — Phase 3 自测：乱序缓冲 / gap 恢复 / UDP 组播端到端
 *
 * 场景覆盖：
 *   1. 乱序窗口内自动理顺
 *   2. 重复包丢弃
 *   3. 大 gap 触发恢复请求 + 补包 + drain
 *   4. GapFill 恢复推进
 *   5. 超窗触发恢复
 *   6. UDP 组播端到端（lo 自发自收，回放=实盘同码路径）
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "feed/feed_handler.hpp"
#include "net/udp_socket.hpp"
#include "protocol/codec.hpp"

using namespace hft;

static int g_fails = 0;
#define CHECK(cond, name) do { \
    if (!(cond)) { ++g_fails; std::printf("FAIL  %s\n", name); } \
    else { std::printf("PASS  %s\n", name); } } while (0)

// ---------- 测试工具：编码器 + 记录型 sink ----------

static std::vector<std::uint8_t> pack(SeqNum seq, proto::MsgType type = proto::MsgType::NewOrder) {
    proto::Message m{};
    m.type = type;
    m.hdr.seq = seq;
    m.hdr.ts = 1000 + seq;
    m.new_order = proto::NewOrder{ .id = 100 + seq, .symbol = 1,
                                   .price = 10000, .qty = 10, .side = Side::Buy };
    if (type == proto::MsgType::GapFill) m.gap_fill = proto::GapFill{ .from = seq, .to = seq };
    std::vector<std::uint8_t> buf(64);
    const std::size_t n = proto::encode(buf.data(), buf.size(), m);
    buf.resize(n);
    return buf;
}

static std::vector<std::uint8_t> concat(std::vector<std::vector<std::uint8_t>> parts) {
    std::vector<std::uint8_t> out;
    for (auto& p : parts) out.insert(out.end(), p.begin(), p.end());
    return out;
}

struct Recorder {
    std::vector<SeqNum> seqs;
    void operator()(const proto::Message& m) { seqs.push_back(m.hdr.seq); }
    bool in_order() const {
        for (size_t i = 1; i < seqs.size(); ++i)
            if (seqs[i] != seqs[i - 1] + 1) return false;
        return true;
    }
};

// ---------- 场景 ----------

static void test_reorder_in_window() {
    Recorder rec;
    feed::FeedHandler<Recorder&> feed(rec);
    // 乱序：1,2,4,3,5（4 先进缓冲，3 到了一起理顺）
    for (auto seq : {1, 2, 4, 3, 5}) {
        auto d = pack(seq);
        feed.on_datagram(d.data(), d.size());
    }
    CHECK(rec.seqs.size() == 5 && rec.in_order() && rec.seqs[0] == 1 && rec.seqs[4] == 5,
          "reorder-in-window");
    CHECK(feed.stats().reordered == 1, "reorder-in-window(reordered==1)");
    CHECK(feed.state() == feed::FeedState::Normal, "reorder-in-window(no recovery)");
}

static void test_duplicate_dropped() {
    Recorder rec;
    feed::FeedHandler<Recorder&> feed(rec);
    for (auto seq : {1, 2, 2, 3}) {
        auto d = pack(seq);
        feed.on_datagram(d.data(), d.size());
    }
    CHECK(rec.seqs.size() == 3 && rec.in_order(), "duplicate-dropped");
    CHECK(feed.stats().duplicates == 1, "duplicate-dropped(dup==1)");
}

static void test_big_gap_recover() {
    Recorder rec;
    SeqNum req_from = 0;
    feed::FeedHandler<Recorder&> feed(rec,
        [&](SeqNum from, SeqNum) { req_from = from; });

    feed.on_datagram(pack(1).data(), pack(1).size());
    feed.on_datagram(pack(2).data(), pack(2).size());
    // 制造超过水位（32）的大 gap：灌 40 条乱序（expect=3，seq 4..43）
    auto d = concat([&] {
        std::vector<std::vector<std::uint8_t>> parts;
        for (SeqNum s = 4; s <= 43; ++s) parts.push_back(pack(s));
        return parts;
    }());
    feed.on_datagram(d.data(), d.size());

    CHECK(feed.state() == feed::FeedState::Recovering, "big-gap(recovering)");
    CHECK(req_from == 3, "big-gap(retransmit request from=3)");
    CHECK(rec.seqs.size() == 2, "big-gap(断档期不多交付)");

    // 恢复通道补来 seq=3，然后 finish_recovery：缓冲里的 4..43 应连续冲出
    auto r3 = pack(3);
    feed.on_retransmit_message([&] {
        proto::Message m;
        proto::decode(r3.data(), r3.size(), m);
        return m;
    }());
    feed.finish_recovery();
    CHECK(rec.seqs.size() == 43 && rec.in_order(), "big-gap(恢复后 1..43 连续)");
    CHECK(feed.stats().recovered_msgs == 1, "big-gap(recovered==1)");
    CHECK(feed.state() == feed::FeedState::Normal, "big-gap(回 Normal)");
}

static void test_gap_fill_recover() {
    Recorder rec;
    feed::FeedHandler<Recorder&> feed(rec);
    feed.on_datagram(pack(1).data(), pack(1).size());
    // 断档触发恢复（灌 40 条乱序 seq 3..42，expect=2）
    auto d = concat([&] {
        std::vector<std::vector<std::uint8_t>> parts;
        for (SeqNum s = 3; s <= 42; ++s) parts.push_back(pack(s));
        return parts;
    }());
    feed.on_datagram(d.data(), d.size());
    CHECK(feed.state() == feed::FeedState::Recovering, "gapfill(recovering)");

    // 重传服务器说 [2,2] 没有业务消息（心跳类跳号）→ GapFill 推进
    CHECK(feed.on_retransmit_gap_fill(2, 2), "gapfill(accepted)");
    feed.finish_recovery();
    CHECK(rec.seqs.size() == 41 && rec.seqs.back() == 42, "gapfill(3..42 冲出)");
    CHECK(feed.stats().gap_fills == 1, "gapfill(count==1)");
}

static void test_out_of_window() {
    Recorder rec;
    bool req = false;
    feed::FeedHandler<Recorder&> feed(rec,
        [&](SeqNum, SeqNum) { req = true; });
    feed.on_datagram(pack(1).data(), pack(1).size());
    auto d = pack(1 + 4096 + 1);  // 窗口 4096 之外
    feed.on_datagram(d.data(), d.size());
    CHECK(req && feed.state() == feed::FeedState::Recovering, "out-of-window(触发恢复)");
}

static void test_udp_end_to_end() {
    // lo 组播自发自收：socket 收到的字节 → on_datagram → 顺序交付
    // （与上面内存直灌完全同一条管线——回放=实盘同码路径的实证）
    const char* GROUP = "239.1.2.3";
    const std::uint16_t PORT = 50123;

    net::UdpSocket rx;
    if (rx.join_multicast(GROUP, PORT, "127.0.0.1") != 0) {
        std::printf("SKIP  udp-end-to-end（lo 组播不可用）\n");
        return;
    }

    Recorder rec;
    feed::FeedHandler<Recorder&> feed(rec);

    // 乱序发 1,2,4,3,5（两条数据报：{1,2,4} 与 {3,5}）
    auto d1 = concat({pack(1), pack(2), pack(4)});
    auto d2 = concat({pack(3), pack(5)});
    net::UdpSocket::send_to(d1.data(), (int)d1.size(), GROUP, PORT, "127.0.0.1");
    net::UdpSocket::send_to(d2.data(), (int)d2.size(), GROUP, PORT, "127.0.0.1");

    std::uint8_t buf[2048];
    int got = 0;
    for (int i = 0; i < 100 && got < 2; ++i) {   // 最多等 ~1s
        const int n = rx.recv(buf, sizeof(buf));
        if (n > 0) { feed.on_datagram(buf, (size_t)n); ++got; }
        else usleep(10000);
    }
    CHECK(got == 2, "udp-end-to-end(收到两条数据报)");
    CHECK(rec.seqs.size() == 5 && rec.in_order() && rec.seqs[4] == 5,
          "udp-end-to-end(乱序理顺)");
}

int main() {
    test_reorder_in_window();
    test_duplicate_dropped();
    test_big_gap_recover();
    test_gap_fill_recover();
    test_out_of_window();
    test_udp_end_to_end();
    std::printf("%s\n", g_fails ? "test_feed FAILED" : "test_feed OK");
    return g_fails;
}
