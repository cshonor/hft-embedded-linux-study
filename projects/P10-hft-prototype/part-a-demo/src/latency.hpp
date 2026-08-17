#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

/*
 * 把每次「处理一笔」花的纳秒记下来，最后排序看分位。
 *
 * 为什么不只看平均值？偶尔一次卡住（排队、操作系统抢核）会把均值拉高，
 * 但你更关心「大部分时候」和「最倒霉的那 1%」。
 *
 * p50  = 一半样本不超过这个数（典型值）
 * p99  = 99% 不超过（尾延迟）
 * p999 = 99.9%
 *
 * 这是进程内计时，不是交易所 T2T，不能拿去和实盘比。
 */
class LatencyHist {
    std::vector<std::uint64_t> samples_;
    std::uint64_t sum_ = 0;
    std::size_t cap_;

public:
    explicit LatencyHist(std::size_t cap = 100000) : cap_(cap) {
        samples_.reserve(cap); // 预先要好内存，热路径 add() 不再 malloc
    }

    void add(std::uint64_t ns) {
        if (samples_.size() >= cap_) {
            return;
        }
        samples_.push_back(ns);
        sum_ += ns;
    }

    std::size_t count() const { return samples_.size(); }

    std::uint64_t percentile(double p) const {
        if (samples_.empty()) {
            return 0;
        }
        std::vector<std::uint64_t> s = samples_;
        std::sort(s.begin(), s.end());
        const double idx = p * static_cast<double>(s.size() - 1);
        const std::size_t i = static_cast<std::size_t>(idx);
        return s[i];
    }

    std::uint64_t mean() const {
        if (samples_.empty()) {
            return 0;
        }
        return sum_ / samples_.size();
    }

    void print(const char* title) const {
        std::printf("  %s  n=%zu  mean=%llu ns  p50=%llu  p99=%llu  p999=%llu\n",
                    title,
                    samples_.size(),
                    static_cast<unsigned long long>(mean()),
                    static_cast<unsigned long long>(percentile(0.50)),
                    static_cast<unsigned long long>(percentile(0.99)),
                    static_cast<unsigned long long>(percentile(0.999)));
    }
};
