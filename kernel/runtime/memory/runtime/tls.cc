#include "runtime/tls.hh"
#include "util/elf64.hh"
#include "runtime/brk.hh"
#include "app/mlog.hh"
#include "util/alignments.hh"

namespace mythos {

namespace {
	static const constexpr uint64_t ELF_PTR = 0x400000;
}

void* handleTLSHeader(const elf64::PHeader *ph) {
	ASSERT(ph->type == elf64::Type::PT_TLS);
	MLOG_DETAIL(mlog::app, "Load TLS", DVARhex(ph), DVARhex(ph->offset), DVARhex(ph->filesize), DVARhex(ph->vaddr), DVARhex(ph->memsize));

	auto tlsAllocationSize = AlignmentObject(ph->alignment).round_up(ph->memsize);

	// TLS Initial Thread Image
	char *initial = (char*) sbrk(tlsAllocationSize);
	memset(initial, 0, tlsAllocationSize);
	memcpy(initial, (void*)(ph->vaddr), ph->filesize);
	initial += tlsAllocationSize;
	*(uint64_t*)initial = (uint64_t) initial; // spec says pointer to itself must be at begin of user structure / end of tls

	return initial;
}

void* setupInitialTLS() {
	mythos::elf64::Elf64Image img((void*) ELF_PTR);
	for (uint64_t i = 0; i < img.phnum(); i++) {
		auto *pheader = img.phdr(i);
		if (pheader->type == elf64::Type::PT_TLS) {
			// in static non-reloc elf without dynamic module loading we have just one tls header
			return handleTLSHeader(pheader);
		}
	}
	return nullptr;
}

} // namespace mythos