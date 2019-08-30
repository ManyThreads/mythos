#pragma once

#include <cstddef>
#include <cstdint>
#include "util/compiler.hh"
#include "boot/bootparam.h"

namespace mythos{
	namespace boot{

extern uint64_t* devices_pml1;
extern uint64_t* devices_pml2;
extern uint64_t* image_pml2;
extern uint64_t* pml2_tables;
extern uint64_t* pml3_table;
extern uint64_t* pml4_table SYMBOL("BOOT_PML4");


extern unsigned long boot_param_pa;
extern struct smp_boot_param *boot_param;
extern int boot_param_size;

extern unsigned long bootstrap_mem_end;
extern unsigned long x86_kernel_phys_base;
extern unsigned long ap_trampoline;
extern unsigned int ihk_ikc_irq;
extern unsigned int ihk_ikc_irq_apicid;
extern uint64_t* pml4_ihk;

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

void memcpy_ringbuf(char const * buf, size_t len);
void kputs(char const *buf);
void putHex(uint64_t ul);

unsigned long virt_to_phys(void *v);
void* phys_to_virt(unsigned long ptr);

/*
 * variable section in the IHK trampoline
 */
struct PACKED trampoline_t {
  uint64_t jump_intr; // .org 8
  uint64_t header_pgtbl;
  uint64_t header_load;
  uint64_t stack_ptr;
  uint64_t notify_addr;
};

} // namespace boot
} // namespace mythos
