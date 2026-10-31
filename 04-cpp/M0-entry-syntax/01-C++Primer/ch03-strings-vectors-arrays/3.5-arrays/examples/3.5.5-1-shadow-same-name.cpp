// 3.5.5-1  同名 data 三处：遮蔽（shadow）与形参退化
// 对应笔记：3.5.5 常见错误 4「同名 ≠ 同一个东西：名字遮蔽」
//
// 编译运行：
//   clang++ -std=c++17 -o shadow 3.5.5-1-shadow-same-name.cpp && ./shadow
//
// 想看「越界被抓现行」（需要 sanitizer）：
//   clang++ -std=c++17 -g -fsanitize=address -DDEMO_OOB -o shadow_oob 3.5.5-1-shadow-same-name.cpp
//   ./shadow_oob        # AddressSanitizer: stack-buffer-overflow
//
// 注：下面 main 里那个 std::array 和 C 数组只能**二选一地存在于同一作用域**，
//     要两个都写、且都叫 data，必须靠内层 {} 块制造新作用域（见 main 里第 ② 处）。

#include <array>
#include <cstdio>

// 第三个 data：形参。写成 int data[5] 会退化成 int*（不是数组、不拷贝元素）
static void PrintArray(int data[5]) {
    printf("  [③ 形参] sizeof=%zu  指向=%p  data[0]=%d\n",
           sizeof(data), (void*)data, data[0]);
}

// 对照：std::array 按值传参 → 真拷贝，是另一块内存
static void PrintArrayByValue(std::array<int, 5> data) {
    printf("  [④ 值传] sizeof=%zu  &data=%p  data[0]=%d\n",
           sizeof(data), (void*)&data, data[0]);
}

int main() {
    std::array<int, 5> data{};        // ① main 作用域
    data[0] = 2;
    data[4] = 1;
    printf("  [① std::array] sizeof=%zu  &data=%p  data.data()=%p  data[0]=%d\n",
           sizeof(data), (void*)&data, (void*)data.data(), data[0]);

    PrintArray(data.data());          // 形参只是指针，指向 ① —— 同一块内存
    PrintArrayByValue(data);          // 值传递 —— 真拷贝，地址不同

    {
        int data[5] = {};             // ② 内层块：合法遮蔽外层 ①
        data[0] = 0;
        printf("  [② C 数组]    sizeof=%zu  &data=%p  data[0]=%d\n",
               sizeof(data), (void*)data, data[0]);
        printf("     ↑ 在②之后、②的块内，写 data 全指②；①被藏起来了\n");
    }

    printf("  出了②的块：data 又回到①  data[0]=%d data[4]=%d\n", data[0], data[4]);

#ifdef DEMO_OOB
    data[5] = 2;                      // ❌ 越界：5 个元素合法下标只有 0..4
    printf("  越界写完了（不开 sanitizer 时看起来没事）: data[5]=%d\n", data[5]);
#endif
    return 0;
}

// ===== 同一作用域写两个 data 会怎样？（把这段复制出去单独编译试试）=====
//   std::array<int, 5> data;
//   int data[5];
// → error: redefinition of 'data' with a different type: 'int[5]' vs 'std::array<int, 5>'
//   遮蔽只在**内层作用域**成立，同一层是重复定义，直接编译失败。
