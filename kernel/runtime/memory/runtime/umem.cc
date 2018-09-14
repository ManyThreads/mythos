#include "runtime/umem.hh"
#include "runtime/SequentialHeap.hh"
//#include <cstdlib>
#include <cstddef>
#include <cstring>

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
    mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}

void operator delete[](void* ptr) NOEXCEPT(true) {
    mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}

extern "C" int posix_memalign(void **memptr, size_t alignment, size_t size) {
    auto tmp = mythos::heap.alloc(size, alignment);
    if (!tmp) return -1;
    *memptr = reinterpret_cast<void*>(*tmp);
    return 0;
}

extern "C" void *calloc(size_t nmemb, size_t size) {
    void* tmp = malloc(nmemb*size);
    if (tmp != nullptr) memset(tmp, 0, nmemb*size);
    return tmp;
}

extern "C" void free(void *ptr) {
    if (ptr != nullptr) mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}

extern "C" void *malloc(size_t size) {
    if (size == 0) return nullptr;
    auto tmp = mythos::heap.alloc(size);
    if (!tmp) return nullptr;
    return reinterpret_cast<void*>(*tmp);
}

extern "C" void *realloc(void *ptr, size_t size) {
    if (ptr == nullptr) return malloc(size); // nothing to copy, behave like malloc()
    auto oldsize = mythos::heap.getSize(ptr);
    if (size == oldsize) return ptr; // size did not change
    void* n = malloc(size);
    if (n == nullptr) {
        if (size == 0) free(ptr); // should behave like free()
        return nullptr;
    }
    if (size > oldsize) memcpy(n, ptr, oldsize);
    else memcpy(n, ptr, size);
    return n;
}
