// 16.2 模板参数与成员模板 —— 非类型参数：编译期定尺寸的环形缓冲
// 实测：clang++ -std=c++20 -Wall 16.2-template-params.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <cstddef>

// N 不是类型，是编译期常量 —— 尺寸烙进类型里
template <typename T, std::size_t N>
class RingBuffer {
public:
    void push(const T& v) {
        buf_[head_] = v;
        head_ = (head_ + 1) % N;
        if (count_ < N) ++count_;
    }
    std::size_t count() const { return count_; }
private:
    T           buf_[N];    // 直接内嵌，一次分配都没有
    std::size_t head_  = 0;
    std::size_t count_ = 0;
};

int main() {
    RingBuffer<long, 4> tiny;       // 4 槽
    RingBuffer<long, 1024> book;    // 1024 槽

    for (int i = 1; i <= 6; ++i) tiny.push(i);   // 超过 4 个，覆盖旧值
    book.push(42);

    printf("4 槽环形缓冲: 装了 6 个后 count = %zu（旧的被覆盖）\n", tiny.count());
    printf("1024 槽环形缓冲: count = %zu\n", book.count());

    // 非类型参数的物理意义：N 进了类型，尺寸编译期已知
    printf("\nsizeof(RingBuffer<long,4>)    = %zu\n", sizeof(tiny));
    printf("sizeof(RingBuffer<long,1024>) = %zu\n", sizeof(book));
    // 两者是不同类型！一个 4 槽缓冲不能赋给 1024 槽变量 ——
    // 这正是"用类型系统消灭运行期错误"的样板

    // HFT 关联：行情窗口、滑点统计、固定槽位对象池全是这个形状。
    // 对比运行期尺寸版本（构造函数里 new 一块 N 大小的内存）：
    //   编译期版：零堆分配、无间接寻址、数组边界可被优化器利用
    //   代价：每种 N 铸一份代码，滥用会撑大二进制（模板膨胀）
    printf("\n非类型参数 = 把运行期的可变尺寸钉死在编译期。\n");
    return 0;
}
