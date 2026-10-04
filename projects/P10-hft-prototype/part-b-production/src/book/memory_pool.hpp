#pragma once

/*
 * memory_pool.hpp — 固定容量 slab 池。
 *
 * 生产纪律：
 *   - 构造时一次性分配（init-time malloc 是允许的），热路径 alloc/free
 *     是纯栈操作，零系统调用、零页错误风险；
 *   - 耗尽返回 nullptr 而不是抛异常/扩容——扩容意味着热路径上的一次
 *     不可预测停顿，生产宁可拒单+告警，也不接受这次停顿；
 *   - 不做对象构造/析构（POD 语义），调用方自行初始化字段；
 *     撮合引擎的 Order 节点就是平凡类型，构造开销为零。
 */

#include <cstddef>
#include <new>

#include "common/compiler.hpp"

namespace hft {

template <typename T>
class Pool {
public:
    explicit Pool(std::size_t capacity) : cap_(capacity) {
        slots_ = static_cast<T*>(::operator new[](sizeof(T) * capacity,
                                                  std::align_val_t{kCacheLine}));
        free_  = static_cast<T**>(::operator new[](sizeof(T*) * capacity));
        // 反向入栈：首次 alloc 按地址递增顺序取用（cache 友好）。
        for (std::size_t i = 0; i < capacity; ++i) {
            free_[i] = &slots_[capacity - 1 - i];
        }
        nfree_ = capacity;
    }

    ~Pool() {
        ::operator delete[](slots_, std::align_val_t{kCacheLine});
        ::operator delete[](free_);
    }

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    HFT_FORCE_INLINE T* alloc() noexcept {
        if (HFT_UNLIKELY(nfree_ == 0)) return nullptr;
        return free_[--nfree_];
    }

    HFT_FORCE_INLINE void free(T* p) noexcept {
        free_[nfree_++] = p;
    }

    [[nodiscard]] std::size_t available() const noexcept { return nfree_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return cap_; }

private:
    T*          slots_;
    T**         free_;
    std::size_t cap_;
    std::size_t nfree_ = 0;
};

} // namespace hft
