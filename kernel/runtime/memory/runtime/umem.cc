#include "runtime/umem.hh"
#include "runtime/SequentialHeap.hh"
//#include <cstdlib>
#include <cstddef>

namespace mythos {
    SequentialHeap<uintptr_t> heap;
} // namespace mythos

void* operator new(std::size_t size) NOEXCEPT(true) {
    auto tmp = mythos::heap.alloc(size, alignof(std::max_align_t));
    if (!tmp) PANIC_MSG(false, "Could not serve memory request."); // should throw bad_alloc
    return reinterpret_cast<void*>(*tmp);
}

void* operator new[](std::size_t size) NOEXCEPT(true) {
    auto tmp = mythos::heap.alloc(size, alignof(std::max_align_t));
    if (!tmp) PANIC_MSG(false, "Could not serve memory request."); // should throw bad_alloc
    return reinterpret_cast<void*>(*tmp);
}

void operator delete(void* ptr) NOEXCEPT(true) {
    if(ptr) mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}

void operator delete[](void* ptr) NOEXCEPT(true) {
    mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}

namespace mythos {
    int posix_memalign(void **memptr, size_t alignment, size_t size) {
        auto tmp = mythos::heap.alloc(size, alignment);
        if (!tmp) return -1;
        *memptr = reinterpret_cast<void*>(*tmp);
        return 0;
    }
}
