#include <iostream>

extern "C" void c_add(int a, int b, int *out);

int main(void)
{
    int sum = 0;
    c_add(3, 5, &sum);
    std::cout << "extern C c_add(3,5)=" << sum << "\n";
    return 0;
}
