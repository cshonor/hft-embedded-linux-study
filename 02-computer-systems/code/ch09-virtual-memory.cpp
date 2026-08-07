/*
 * CSAPP Ch9 · 虚拟内存 — C++ 版 (RAII 包装 mmap)
 *
 * 对照笔记:
 *   chapter-09/notes/section-9.8-内存映射mmap.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch09_vm_cpp ch09-virtual-memory.cpp
 * 运行:
 *   ./ch09_vm_cpp
 *
 * C++ 差异:
 *   - RAII: MappedFile 析构自动 munmap, 无资源泄漏
 *   - unique_ptr + 自定义 deleter 管理页对齐内存
 *   - 异常安全: 构造函数失败抛异常而非返回 NULL
 *   - constexpr 页大小
 */

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// ---------- RAII: mmap 匿名映射 ----------
class MappedMemory {
    void*   ptr_;
    size_t  size_;
public:
    MappedMemory(size_t size, int prot = PROT_READ | PROT_WRITE)
        : ptr_(nullptr), size_(size)
    {
        ptr_ = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr_ == MAP_FAILED)
            throw std::runtime_error("mmap failed: " + std::string(strerror(errno)));
    }
    ~MappedMemory() {
        if (ptr_ && ptr_ != MAP_FAILED)
            munmap(ptr_, size_);
    }
    // 禁止拷贝 (mmap 资源唯一所有权)
    MappedMemory(const MappedMemory&) = delete;
    MappedMemory& operator=(const MappedMemory&) = delete;
    // 允许移动
    MappedMemory(MappedMemory&& o) noexcept : ptr_(o.ptr_), size_(o.size_) {
        o.ptr_ = nullptr; o.size_ = 0;
    }

    void*       data()       { return ptr_; }
    const void* data() const { return ptr_; }
    size_t      size() const { return size_; }

    // 设为只读
    void set_readonly() {
        if (mprotect(ptr_, size_, PROT_READ) != 0)
            throw std::runtime_error("mprotect failed");
    }
};

// ---------- RAII: 文件映射 ----------
class MappedFile {
    void*  ptr_;
    size_t size_;
public:
    MappedFile(const char* path, int prot = PROT_READ)
        : ptr_(nullptr), size_(0)
    {
        int flags = (prot & PROT_WRITE) ? MAP_SHARED : MAP_PRIVATE;
        int fd = open(path, O_RDWR);
        if (fd < 0) throw std::runtime_error("open failed");

        struct stat st;
        if (fstat(fd, &st) != 0) { close(fd); throw std::runtime_error("fstat"); }
        size_ = st.st_size;

        ptr_ = mmap(nullptr, size_, prot, flags, fd, 0);
        close(fd);
        if (ptr_ == MAP_FAILED) throw std::runtime_error("mmap failed");
    }
    ~MappedFile() {
        if (ptr_ && ptr_ != MAP_FAILED) munmap(ptr_, size_);
    }
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    const char* data() const { return static_cast<const char*>(ptr_); }
    size_t      size() const { return size_; }
};

// ---------- 页对齐分配 (unique_ptr + 自定义 deleter) ----------
struct PageAlignedDeleter {
    size_t size;
    void operator()(void* p) const { free(p); }
};
using PageAlignedPtr = std::unique_ptr<void, PageAlignedDeleter>;

static PageAlignedPtr alloc_page_aligned(size_t size)
{
    void* p = nullptr;
    long ps = sysconf(_SC_PAGESIZE);
    if (posix_memalign(&p, ps, size) != 0)
        throw std::runtime_error("posix_memalign failed");
    return PageAlignedPtr(p, PageAlignedDeleter{size});
}

// ------------------------------------------------------------------
int main()
{
    long page_size = sysconf(_SC_PAGESIZE);
    printf("=== CSAPP Ch9 · 虚拟内存 C++ (PAGE_SIZE=%ld) ===\n\n", page_size);

    // 1. RAII 匿名映射 — 析构自动 munmap
    printf("--- 1. RAII 匿名映射 ---\n");
    {
        MappedMemory mem(page_size * 4);
        char* p = static_cast<char*>(mem.data());
        snprintf(p, 64, "Hello from MappedMemory!");
        printf("  写入: \"%s\"  size=%zu\n", p, mem.size());
    } // 析构自动 munmap — 离开作用域即释放
    printf("  离开作用域 → 自动 munmap ✓\n\n");

    // 2. 页对齐分配 (unique_ptr)
    printf("--- 2. 页对齐分配 (unique_ptr + deleter) ---\n");
    {
        auto ptr = alloc_page_aligned(page_size * 2);
        printf("  ptr=%p  对齐=%ldB\n", ptr.get(), page_size);
    } // 自动 free
    printf("  离开作用域 → 自动 free ✓\n\n");

    // 3. 文件映射 (RAII)
    printf("--- 3. 文件映射 (RAII) ---\n");
    {
        const char* tmpfile = "/tmp/ch09_mmap_cpp.txt";
        FILE* fp = fopen(tmpfile, "w");
        if (fp) { fputs("mmap file content via RAII\n", fp); fclose(fp); }

        try {
            MappedFile mf(tmpfile);
            printf("  映射 %zu bytes: \"%.*s\"",
                   mf.size(), (int)mf.size(), mf.data());
        } catch (const std::exception& e) {
            printf("  %s\n", e.what());
        }
        unlink(tmpfile);
    }
    printf("  离开作用域 → 自动 munmap ✓\n\n");

    // 4. mprotect via RAII
    printf("--- 4. mprotect (RAII 方法) ---\n");
    {
        MappedMemory mem(page_size, PROT_READ | PROT_WRITE);
        char* p = static_cast<char*>(mem.data());
        strcpy(p, "可读写");
        printf("  写入: \"%s\" ✓\n", p);

        mem.set_readonly();
        printf("  mprotect → 只读\n");
        printf("  读取: \"%s\" ✓\n", p);
        printf("  写入: 会 SIGSEGV (已注释)\n");
        // p[0] = 'X';  // → SIGSEGV
    }
    printf("\n");

    // 5. 大页
#ifdef __linux__
    printf("--- 5. 大页 (MAP_HUGETLB) ---\n");
    {
        size_t sz = 2 * 1024 * 1024;
        void* p = mmap(nullptr, sz, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if (p != MAP_FAILED) {
            memset(p, 0, sz);
            printf("  2MB 大页 ✓  ptr=%p\n", p);
            printf("  普通: 512 TLB 条目 → 大页: 1 TLB 条目\n");
            munmap(p, sz);
        } else {
            printf("  MAP_HUGETLB 失败 (需 echo 20 > /proc/sys/vm/nr_hugepages)\n");
        }
    }
    printf("\n");
#endif

    printf("C++ 特有点:\n");
    printf("  - RAII: MappedFile/MappedMemory 析构自动释放, 无泄漏风险\n");
    printf("  - 异常安全: 构造失败抛异常, 不会返回半初始化对象\n");
    printf("  - unique_ptr + 自定义 deleter 管理页对齐内存\n");
    printf("  - 禁止拷贝 + 允许移动: mmap 资源唯一所有权语义\n");

    return 0;
}
