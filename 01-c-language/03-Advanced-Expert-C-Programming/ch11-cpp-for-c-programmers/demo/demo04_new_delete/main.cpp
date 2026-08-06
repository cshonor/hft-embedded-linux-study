#include <cstdlib>
#include <iostream>
#include <new>

int main(void)
{
    int *p = static_cast<int *>(std::malloc(sizeof(int)));
    if (p) {
        *p = 1;
        std::free(p);
    }

    int *q = new int(42);
    std::cout << "new int: " << *q << "\n";
    delete q;

    int *arr = new int[4]{1, 2, 3, 4};
    std::cout << "new[] arr[3]=" << arr[3] << "\n";
    delete[] arr;

    return 0;
}
