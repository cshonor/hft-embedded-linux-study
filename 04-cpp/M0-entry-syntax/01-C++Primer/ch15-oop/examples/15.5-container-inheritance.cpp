// 15.5 容器与继承 —— 切片灾难 vs 多态容器（object pooling 前置课）
// 实测：clang++ -std=c++20 -Wall 15.5-container-inheritance.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <vector>
#include <memory>

class Event {
public:
    virtual ~Event() = default;
    virtual const char* kind() const { return "Event"; }
};

class Tick : public Event {
public:
    const char* kind() const override { return "Tick（逐笔）"; }
};

class Depth : public Event {
public:
    const char* kind() const override { return "Depth（档位）"; }
};

static void show(const Event& e) { printf("  %s\n", e.kind()); }

int main() {
    // ========== 反面教材：vector<Event> 存值 ==========
    std::vector<Event> by_value;
    by_value.push_back(Tick());     // 只把 Tick 的 Event 部分拷进去 = 切片！
    by_value.push_back(Depth());
    printf("vector<Event>（存值）:\n");
    for (const auto& e : by_value) show(e);
    // 输出全是 Event —— 虚表指针没被拷贝（拷的是 Event 基子物），
    // Tick/Depth 的新增成员直接丢掉。虚机制整个失效。

    // ========== 正确做法：容器存（智能）指针 ==========
    std::vector<std::unique_ptr<Event>> by_ptr;
    by_ptr.push_back(std::make_unique<Tick>());
    by_ptr.push_back(std::make_unique<Depth>());
    printf("\nvector<unique_ptr<Event>>（存指针）:\n");
    for (const auto& e : by_ptr) show(*e);
    // 输出 Tick / Depth —— 对象完整地在堆上，指针只负责指过去。

    // 为什么会这样，一句话：
    //   vector<Event> 要求所有元素同尺寸 -> 只能装下 Event 那部分
    //   vector<Event*> 每个元素 8 字节 -> 指向的对象想多大多大
    //
    // HFT 关联：事件队列/订单簿快照全靠多态容器。
    // 更进一步（等你写内存池时回来看）：
    //   unique_ptr 的 new/delete 可以换成池分配 ——
    //   事件对象高频生灭，堆分配的抖动才是要消灭的目标。
    printf("\n切片 = 值语义容器的天罚；多态容器 = 指针语义。\n");
    return 0;
}
