#include "runtime/tls.hh"
#include "util/elf64.hh"
#include "runtime/brk.hh"
#include "app/mlog.hh"
#include "util/alignments.hh"
#include "runtime/umem.hh"

namespace mythos {

namespace {
	static const constexpr uint64_t ELF_tlsAddr = 0x400000;
}

template<typename ALLOC> // functor to alloc the space for TLS
void* handleTLSHeader(const elf64::PHeader *ph, ALLOC alloc) {
	ASSERT(ph->type == elf64::Type::PT_TLS);

	auto tlsAllocationSize = AlignmentObject(ph->alignment).round_up(ph->memsize);
	// TLS tlsAddr Thread Image
	char *tlsAddr = (char*) alloc(tlsAllocationSize);
	memset(tlsAddr, 0, tlsAllocationSize);
	memcpy(tlsAddr, (void*)(ph->vaddr), ph->filesize);
	tlsAddr += tlsAllocationSize;
	*(uint64_t*)tlsAddr = (uint64_t) tlsAddr; // spec says pointer to itself must be at begin of user structure / end of tls

	MLOG_DETAIL(mlog::app, "Load TLS",DVARhex(tlsAddr), DVARhex(ph), DVARhex(ph->offset), DVARhex(ph->filesize), DVARhex(ph->vaddr), DVARhex(ph->memsize));

	return tlsAddr;
}

void* setupInitialTLS() {
	mythos::elf64::Elf64Image img((void*) ELF_tlsAddr);
	for (uint64_t i = 0; i < img.phnum(); i++) {
		auto *pheader = img.phdr(i);
		if (pheader->type == elf64::Type::PT_TLS) {
			// in static non-reloc elf without dynamic module loading we have just one tls header
			return handleTLSHeader(pheader, sbrk);
		}
	}
	return nullptr;
}

void* setupNewTLS() {
	mythos::elf64::Elf64Image img((void*) ELF_tlsAddr);
	for (uint64_t i = 0; i < img.phnum(); i++) {
		auto *pheader = img.phdr(i);
		if (pheader->type == elf64::Type::PT_TLS) {
			// in static non-reloc elf without dynamic module loading we have just one tls header
			return handleTLSHeader(pheader, [](uint64_t size) { return new char[size]; });
		}
	}
	return nullptr;
}

} // namespace mythos