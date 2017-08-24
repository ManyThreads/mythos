#include "runtime/SequentialHeap.hh"


namespace mythos {

SequentialHeap<uintptr_t> heap;

} // namespace mythos

void* operator new(unsigned long size ) NOEXCEPT(true) {
	auto tmp = mythos::heap.alloc(size);
	if (tmp) {
		return (void*) *tmp;
	}
	return nullptr;
}
void* operator new[](unsigned long size ) NOEXCEPT(true) {
	auto tmp = mythos::heap.alloc(size);
	if (tmp) {
		return (void*) *tmp;
	}
	return nullptr;
}

void operator delete(void* ptr) NOEXCEPT(true) {
	mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}

void operator delete[](void* ptr) NOEXCEPT(true) {
	mythos::heap.free(reinterpret_cast<uintptr_t>(ptr));
}