#include "runtime/tls.hh"
#include "util/elf64.hh"
#include "runtime/brk.hh"
#include "runtime/mlog.hh"
#include "util/alignments.hh"
#include "runtime/umem.hh"
#include "mythos/init.hh"
#include "mythos/protocol/ExecutionContext.hh"
#include "mythos/syscall.hh"
#include "mythos/Error.hh"

extern char __executable_start; //< provided by the default linker script

namespace mythos {

struct TLSControlBlock
{
    TLSControlBlock() { tcb = this; }
    
    TLSControlBlock* tcb;
    void* dtv;
    void* self;
    int   multiple_threads;
    void* sysinfo;
    uintptr_t stack_guard = { 0xDEADBEEFBABE0000};
    uintptr_t pointer_guard;
    int   gscope_flag;
    int   private_futex;
    void* private_tm[5];    
};

template<class T>
inline T max(T a, T b) { return (a>b)?a:b; }

/** find the TLS header.
 *
 * In static non-reloc ELF without dynamic module loading we have just one TLS header.
 */
mythos::elf64::PHeader const* findTLSHeader() {
    mythos::elf64::Elf64Image img(&__executable_start);
    for (uint64_t i = 0; i < img.phnum(); i++) {
        auto pheader = img.phdr(i);
        if (pheader->type == elf64::Type::PT_TLS) return pheader;
    }
    return nullptr;
}

extern mythos::InvocationBuf* msg_ptr asm("msg_ptr");

void setupInitialTLS() {
    // find the TLS program header
    auto ph = findTLSHeader();
    ASSERT(ph != nullptr);
    ASSERT(ph->type == elf64::Type::PT_TLS);
    
    // allocate some memory from sbrk and then copy the TLS segment and control block there
    AlignmentObject align(max(ph->alignment, alignof(TLSControlBlock)));
    auto tlsAllocationSize = align.round_up(ph->memsize) + sizeof(TLSControlBlock);
    auto addr = reinterpret_cast<char*>(sbrk(tlsAllocationSize)); // @todo should be aligned to align.alignment()
    ASSERT(addr != nullptr);
    auto tcbAddr = addr + align.round_up(ph->memsize);
    auto tlsAddr = tcbAddr - AlignmentObject(ph->alignment).round_up(ph->memsize);
    
    ASSERT(ph->filesize <= ph->memsize);
    memset(tlsAddr, 0, ph->memsize);
    memcpy(tlsAddr, (void*)(ph->vaddr), ph->filesize);
    new(tcbAddr) TLSControlBlock(); // placement new to set up the TCB

    // make a system call to set the new FS base address
    msg_ptr->write<protocol::ExecutionContext::SetFSGS>(reinterpret_cast<uintptr_t>(tcbAddr), 0);
    auto result = syscall_invoke_wait(mythos::init::PORTAL, mythos::init::EC, (void*)0xDEADBEEF);
    ASSERT(result.user == (uintptr_t)0xDEADBEEF);
    ASSERT(result.state == (uint64_t)Error::SUCCESS);
}

void* setupNewTLS() {
    auto ph = findTLSHeader();
    if (ph == nullptr || ph->type != elf64::Type::PT_TLS) {
        MLOG_ERROR(mlog::app, "Load TLS found no TLS program header");
        return nullptr;
    }

    AlignmentObject align(max(ph->alignment, alignof(TLSControlBlock)));
    auto tlsAllocationSize = align.round_up(ph->memsize) + sizeof(TLSControlBlock);

    auto tmp = mythos::heap.alloc(tlsAllocationSize, align.alignment());
    if (!tmp) {
        MLOG_ERROR(mlog::app, "Load TLS could not allocate memory from heap",
            DVAR(tlsAllocationSize), DVAR(align.alignment()), tmp.state());
        return nullptr;
    }
    auto addr = reinterpret_cast<char*>(*tmp);

    auto tcbAddr = addr + align.round_up(ph->memsize);
    auto tlsAddr = tcbAddr - AlignmentObject(ph->alignment).round_up(ph->memsize);
    
    ASSERT(ph->filesize <= ph->memsize);
    memset(tlsAddr, 0, ph->memsize);
    memcpy(tlsAddr, (void*)(ph->vaddr), ph->filesize);
    new(tcbAddr) TLSControlBlock(); // placement new to set up the TCB

    MLOG_DETAIL(mlog::app, "Load TLS", DVARhex(tlsAddr), DVARhex(tcbAddr), 
                DVARhex(ph), DVARhex(ph->offset), DVARhex(ph->filesize), 
                DVARhex(ph->vaddr), DVARhex(ph->memsize));
    return tcbAddr;
}

} // namespace mythos
