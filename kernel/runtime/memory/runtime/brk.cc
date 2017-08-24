#include "runtime/brk.hh"
#include <array>

namespace mythos {

namespace {
	ALIGNED(64) std::array<char,8 * 1024 * 1024> bss;
	uint64_t idx = 0;
}

void* sbrk(uint64_t size) {
	ASSERT(idx + size < bss.size());
	if (size == 0) return &bss[idx];
	auto old = &bss[idx];
	idx += size;
	return (void*)old;
}

} // namespace mythos