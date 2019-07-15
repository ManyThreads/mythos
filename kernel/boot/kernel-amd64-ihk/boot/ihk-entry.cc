#include "util/compiler.hh"
#include "boot/bootparam.h"

uint64_t* pml4_table SYMBOL("BOOT_PML4");

unsigned long boot_param_pa;
struct smp_boot_param *boot_param;
int boot_param_size;
unsigned long bootstrap_mem_end;
unsigned long x86_kernel_phys_base;
unsigned long ap_trampoline = 0;
unsigned int ihk_ikc_irq = 0;
unsigned int ihk_ikc_irq_apicid = 0;

#define MAP_KERNEL_START   0xFFFFFFFFFE800000UL
#define LINUX_PAGE_OFFSET  0xffff880000000000UL
#define MAP_FIXED_START    0xffff860000000000UL
#define MAP_ST_START       0xffff800000000000UL

void* phys_to_virt(unsigned long ptr){
   return (void*)(ptr + MAP_ST_START);
}

unsigned long virt_to_phys(void *v)
{
	unsigned long va = (unsigned long)v;

	if (va >= MAP_KERNEL_START) {
		return va - MAP_KERNEL_START + x86_kernel_phys_base;
	}
	else if (va >= LINUX_PAGE_OFFSET) {
		return va - LINUX_PAGE_OFFSET;
	}
	else if (va >= MAP_FIXED_START) {
		return va - MAP_FIXED_START;
	}
	else {
		return va - MAP_ST_START;
	}
}

extern char VIRT_END[] SYMBOL("VIRT_END");

char* last_page = 0;

void* alloc_pages(int nr_pages){
	//init
	if(!last_page){
		last_page = VIRT_END;
	}
	void* ret = last_page;
	last_page += nr_pages * 4096;
	return ret;
}

void _start_ihk_mythos_(unsigned long param_addr, unsigned long phys_address, 
	unsigned long _ap_trampoline) SYMBOL("_start_ihk_mythos_");

void _start_ihk_mythos_(unsigned long param_addr, unsigned long phys_address, 
	unsigned long _ap_trampoline)
{
	x86_kernel_phys_base = phys_address;
	boot_param = static_cast<smp_boot_param*>(phys_to_virt(param_addr));
	boot_param_pa = param_addr;
	ap_trampoline = _ap_trampoline;
	ihk_ikc_irq = boot_param->ihk_ikc_irq;
	bootstrap_mem_end = boot_param->bootstrap_mem_end;
	boot_param_size = boot_param->param_size;

	void* rq = alloc_pages(1); 
	void* wq = alloc_pages(1); 

	boot_param->mikc_queue_recv = virt_to_phys(wq);
	boot_param->mikc_queue_send = virt_to_phys(rq);

	//boot_param->status = 1;
	//asm volatile("" ::: "memory");
	
	/* get ready */
	boot_param->status = 2;
	asm volatile("" ::: "memory");

	while(1);
}
