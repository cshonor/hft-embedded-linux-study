// 12.1 weak_ptr：expired() + lock() 的竞态（确定性复现，不靠运气）
//
// 编译运行：
//   clang++ -std=c++17 12.1-weak-ptr-race.cpp -o /tmp/race && /tmp/race        # 正确写法（默认）
//   /tmp/race bad                                                              # 错误写法（必崩）
//
// 为什么"必崩"：两个线程之间用两个 atomic 标志把顺序钉死——
//   victim 先完成 expired() 检查 → 通知 killer → killer reset → 通知 victim → victim 才 lock
// 于是"检查完之后对象才被释放"这件事是 100% 发生的，不是概率事件。

#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>

static void scenario(bool safe) {
    auto sp = std::make_shared<int>(42);
    std::weak_ptr<int> wp = sp;
    std::atomic<bool> checked{false};    // victim 已完成检查
    std::atomic<bool> released{false};   // killer 已释放对象

    std::thread victim([&] {
        checked = true;                  // 两种写法都从这里起步
        if (safe) {
            // ✅ 正确写法：检查与获取合并成一次原子 lock()
            while (!released) {}         // 故意等对方先释放，仍然安全
            if (auto s = wp.lock())
                std::printf("  [正确写法] lock() 成功，*s = %d\n", *s);
            else
                std::printf("  [正确写法] lock() 返回空 —— 对象已释放，安全避开\n");
        } else {
            // ❌ 错误写法：先 expired() 再 lock()，中间有窗口
            if (!wp.expired()) {         // ① 此刻还活着
                while (!released) {}     // ② 窗口（这里被 killer 抢先释放）
                auto s = wp.lock();      // ③ 已经太晚：返回空 shared_ptr
                std::printf("  [错误写法] *s = %d\n", *s);   // ④ 空指针解引用
            }
        }
    });

    std::thread killer([&] {
        while (!checked) {}              // 等 victim 完成检查
        sp.reset();                      // 在窗口内把最后一个 shared_ptr 释放掉
        released = true;
    });

    victim.join();
    killer.join();
}

int main(int argc, char** argv) {
    bool safe = !(argc > 1 && std::strcmp(argv[1], "bad") == 0);
    if (safe) {
        std::printf("== 正确写法：一次 lock()，检查与获取是原子的 ==\n");
        scenario(true);
        std::printf("  进程正常结束。\n");
    } else {
        std::printf("== 错误写法：expired() 之后再 lock() ==\n");
        scenario(false);
        std::printf("  这行打不出来 —— 上面已经段错误了。\n");
    }
    return 0;
}
