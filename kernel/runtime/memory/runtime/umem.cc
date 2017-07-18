#include "runtime/SequentialHeap.hh"


mythos::SequentialHeap<uintptr_t> heap;

void* operator new(unsigned long size ) NOEXCEPT(true) {
	auto tmp = heap.alloc(size, heap.getAlignment());
	if (tmp) {
		return (void*) *tmp;
	}
	return nullptr;
}
void* operator new[](unsigned long size ) NOEXCEPT(true) {
	auto tmp = heap.alloc(size, heap.getAlignment());
	if (tmp) {
		return (void*) *tmp;
	}
	return nullptr;
}

void operator delete(void* ptr) NOEXCEPT(true) {
	heap.free(reinterpret_cast<uintptr_t>(ptr));
}

void operator delete[](void* ptr) NOEXCEPT(true) {
	heap.free(reinterpret_cast<uintptr_t>(ptr));
}