#pragma once

#include "boot/bootparam.h"

namespace mythos{
	namespace boot{

extern unsigned long boot_param_pa;
extern struct smp_boot_param *boot_param;
extern int boot_param_size;

extern unsigned long bootstrap_mem_end;
extern unsigned long x86_kernel_phys_base;
extern unsigned long ap_trampoline;
extern unsigned int ihk_ikc_irq;
extern unsigned int ihk_ikc_irq_apicid;

#define MAP_KERNEL_START   0xFFFFFFFFFE800000UL
#define LINUX_PAGE_OFFSET  0xffff880000000000UL
#define MAP_FIXED_START    0xffff860000000000UL
#define MAP_ST_START       0xffff800000000000UL

#define IHK_KMSG_SIZE            (8192 << 5)
#define DEBUG_KMSG_MARGIN (kmsg_buf->head == kmsg_buf->tail ? kmsg_buf->len : (((unsigned int)kmsg_buf->head - (unsigned int)kmsg_buf->tail) % (unsigned int)kmsg_buf->len))


struct ihk_kmsg_buf {
	int lock; /* Be careful, it's inter-kernel lock */
	int tail;
	int len;
	int head;
	char padding[4096 - sizeof(int) * 4]; /* Alignmment needed for some systems */
	char str[IHK_KMSG_SIZE];
};

extern struct ihk_kmsg_buf *kmsg_buf;

extern void memcpy_ringbuf(char const * buf, size_t len);
extern void kputs(char const *buf);
extern void putHex(uint64_t ul);

unsigned long virt_to_phys(void *v);
void* phys_to_virt(unsigned long ptr);
}

//inline constexpr uint64_t indicesToVirt(unsigned pml4, unsigned pml3, unsigned pml2, unsigned pml1)

}
