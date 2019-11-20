#include "runtime/tls.hh"
#include "util/elf64.hh"
#include "runtime/brk.hh"
#include "runtime/mlog.hh"
#include "util/align.hh"
#include "runtime/umem.hh"
#include "mythos/init.hh"
#include "mythos/protocol/ExecutionContext.hh"
#include "mythos/syscall.hh"
#include "mythos/Error.hh"

extern char __executable_start; //< provided by the default linker script

namespace mythos {

/* Dont change! */
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

/** make a system call to set the new FS base address.
 * Called from musl __init_tls.
 * Original was using set_thread_area system call, but passed the pointer instead of a GDT struct. 
 * Did not match the man page.
 */
extern "C" int __set_thread_area(void *p)
{
    msg_ptr->write<protocol::ExecutionContext::SetFSGS>(reinterpret_cast<uintptr_t>(p), 0);
    auto result = syscall_invoke_wait(mythos::init::PORTAL, mythos::init::EC, (void*)0xDEADBEEF);
    ASSERT(result.user == (uintptr_t)0xDEADBEEF);
    ASSERT(result.state == (uint64_t)Error::SUCCESS);

    return 0;
}

/** allocate memory for the thread local storage.
 * Called from musl __init_tls.
 * Original was using mmap systemcall for anonymous memory at arbitrary position.
 */
extern "C" void* __mythos_get_tlsmem(unsigned long size)
{
    return sbrk(size);
}


void setupInitialTLS() {
    // find the TLS program header
    auto ph = findTLSHeader();
    ASSERT(ph != nullptr);
    ASSERT(ph->type == elf64::Type::PT_TLS);
    
    // allocate some memory from sbrk and then copy the TLS segment and control block there
    auto align = max(ph->alignment, alignof(TLSControlBlock));
    auto tlsAllocationSize = round_up(ph->memsize, align) + sizeof(TLSControlBlock);
    auto addr = reinterpret_cast<char*>(__mythos_get_tlsmem(tlsAllocationSize)); // @todo should be aligned to align
    ASSERT(addr != nullptr);
    auto tcbAddr = addr + round_up(ph->memsize, align);
    auto tlsAddr = tcbAddr - round_up(ph->memsize, ph->alignment);
    
    ASSERT(ph->filesize <= ph->memsize);
    memset(tlsAddr, 0, ph->memsize);
    memcpy(tlsAddr, (void*)(ph->vaddr), ph->filesize);
    new(tcbAddr) TLSControlBlock(); // placement new to set up the TCB

    __set_thread_area(reinterpret_cast<void*>(tcbAddr));
}

void* setupNewTLS() {
    auto ph = findTLSHeader();
    if (ph == nullptr || ph->type != elf64::Type::PT_TLS) {
        MLOG_ERROR(mlog::app, "Load TLS found no TLS program header");
        return nullptr;
    }

    auto align = max(ph->alignment, alignof(TLSControlBlock));
    auto tlsAllocationSize = round_up(ph->memsize, align) + sizeof(TLSControlBlock);

    auto tmp = mythos::heap.alloc(tlsAllocationSize, align);
    if (!tmp) {
        MLOG_ERROR(mlog::app, "Load TLS could not allocate memory from heap",
            DVAR(tlsAllocationSize), DVAR(align), tmp.state());
        return nullptr;
    }
    auto addr = reinterpret_cast<char*>(*tmp);

    auto tcbAddr = addr + round_up(ph->memsize, align);
    auto tlsAddr = tcbAddr - round_up(ph->memsize, ph->alignment);
    
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
