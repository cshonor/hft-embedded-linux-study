#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

// 进程内延迟样本。生产环境会换成 RDTSC + 硬件时间戳；demo 只保证「有分位」。
class LatencyHist {
    std::vector<std::uint64_t> samples_;
    std::uint64_t sum_ = 0;
    std::size_t cap_;

public:
    explicit LatencyHist(std::size_t cap = 100000) : cap_(cap) {
        samples_.reserve(cap);
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
