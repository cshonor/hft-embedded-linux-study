#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

/* P8 最小 LOB：价格优先 + FIFO。完整回放在 P10 / 18-rust-quant。 */

enum class Side { Buy, Sell };

struct Order {
    uint64_t id{};
    Side side{Side::Buy};
    int64_t price{};
    int64_t qty{};
};

struct Trade {
    uint64_t maker_id{};
    int64_t price{};
    int64_t qty{};
};

class Book {
public:
    std::vector<Trade> submit(Order o)
    {
        std::vector<Trade> t;
        if (o.side == Side::Buy) {
            while (o.qty > 0 && !asks.empty() && o.price >= asks.begin()->first) {
                fill_level(o, asks, t);
            }
            if (o.qty > 0)
                rest(o, bids);
        } else {
            while (o.qty > 0 && !bids.empty() && o.price <= bids.rbegin()->first) {
                fill_level_bid(o, t);
            }
            if (o.qty > 0)
                rest(o, asks);
        }
        return t;
    }

    int64_t best_bid() const { return bids.empty() ? -1 : bids.rbegin()->first; }
    int64_t best_ask() const { return asks.empty() ? -1 : asks.begin()->first; }

private:
    struct Level {
        std::deque<Order> q;
    };
    std::map<int64_t, Level> bids;
    std::map<int64_t, Level> asks;

    void rest(const Order &o, std::map<int64_t, Level> &m)
    {
        m[o.price].q.push_back(o);
    }

    void fill_level(Order &taker, std::map<int64_t, Level> &opp, std::vector<Trade> &out)
    {
        auto it = opp.begin();
        auto &q = it->second.q;
        while (taker.qty > 0 && !q.empty()) {
            Order &m = q.front();
            int64_t qn = std::min(taker.qty, m.qty);
            out.push_back(Trade{m.id, it->first, qn});
            taker.qty -= qn;
            m.qty -= qn;
            if (m.qty == 0)
                q.pop_front();
        }
        if (q.empty())
            opp.erase(it);
    }

    void fill_level_bid(Order &taker, std::vector<Trade> &out)
    {
        auto it = std::prev(bids.end());
        auto &q = it->second.q;
        while (taker.qty > 0 && !q.empty()) {
            Order &m = q.front();
            int64_t qn = std::min(taker.qty, m.qty);
            out.push_back(Trade{m.id, it->first, qn});
            taker.qty -= qn;
            m.qty -= qn;
            if (m.qty == 0)
                q.pop_front();
        }
        if (q.empty())
            bids.erase(it);
    }
};

static int fail(const char *n)
{
    std::cerr << "FAIL " << n << "\n";
    return 1;
}

int main()
{
    int fails = 0;
    {
        Book b;
        b.submit(Order{1, Side::Sell, 10400, 50});
        b.submit(Order{2, Side::Sell, 10300, 200});
        auto t = b.submit(Order{3, Side::Buy, 10500, 100});
        if (t.size() != 1 || t[0].price != 10300 || t[0].qty != 100)
            fails += fail("best-price");
        else
            std::cout << "PASS  best-price\n";
    }
    {
        Book b;
        auto a = Order{1, Side::Sell, 10000, 10};
        b.submit(a);
        b.submit(Order{2, Side::Sell, 10000, 10});
        auto t = b.submit(Order{3, Side::Buy, 10000, 10});
        if (t.size() != 1 || t[0].maker_id != 1)
            fails += fail("fifo");
        else
            std::cout << "PASS  fifo\n";
    }
    {
        Book b;
        b.submit(Order{1, Side::Sell, 9900, 10});
        auto t = b.submit(Order{2, Side::Buy, 9800, 10});
        if (!t.empty() || b.best_bid() != 9800 || b.best_ask() != 9900)
            fails += fail("no-match");
        else
            std::cout << "PASS  no-match\n";
    }
    std::cout << (fails ? "P8 LOB FAILED\n" : "part-a-lob: OK\n");
    return fails;
}
