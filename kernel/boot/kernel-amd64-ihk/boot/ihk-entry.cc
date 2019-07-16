#include "util/compiler.hh"
#include "boot/bootparam.h"
#include "boot/pagetables.hh"

namespace mythos{
	namespace boot{
uint64_t* devices_pml1;
uint64_t* devices_pml2;
uint64_t* image_pml2;
uint64_t* pml2_tables;
uint64_t* pml3_table;
uint64_t* pml4_table SYMBOL("BOOT_PML4");

static unsigned char stack[8192] __attribute__((aligned(4096)));

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

#define IHK_KMSG_SIZE            8192
#define DEBUG_KMSG_MARGIN (kmsg_buf->head == kmsg_buf->tail ? kmsg_buf->len : (((unsigned int)kmsg_buf->head - (unsigned int)kmsg_buf->tail) % (unsigned int)kmsg_buf->len))

struct ihk_kmsg_buf {
	int lock; /* Be careful, it's inter-kernel lock */
	int tail;
	int len;
	int head;
	char padding[4096 - sizeof(int) * 4]; /* Alignmment needed for some systems */
	char str[IHK_KMSG_SIZE];
};

struct ihk_kmsg_buf *kmsg_buf;

static void memcpy_ringbuf(char* buf, int len) {
	int i;
	for(i = 0; i < len; i++) {
		*(kmsg_buf->str + kmsg_buf->tail) = *(buf + i);
		kmsg_buf->tail = (kmsg_buf->tail + 1) % kmsg_buf->len;
	}
}

size_t strlen(const char *p)
{
	const char *head = p;

	while(*p){
		p++;
	}
	
	return p - head;
}

void kputs(char *buf)
{
	int len = strlen(buf);
	unsigned long flags_outer, flags_inner;
	int overflow;

	if (kmsg_buf == NULL) {
		return;
	}

	while(__sync_val_compare_and_swap(&kmsg_buf->lock, 0, 1) != 0){
	   asm volatile("pause" ::: "memory");
	}

	overflow = DEBUG_KMSG_MARGIN <= len;

	memcpy_ringbuf(buf, len);
	
	if (overflow) {
		kmsg_buf->head = (kmsg_buf->tail + 1) % kmsg_buf->len;
	}
	kmsg_buf->lock = 0;
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

	kmsg_buf = (struct ihk_kmsg_buf*)boot_param->msg_buffer;

	kputs("Hello from the other side!\n");	

	kputs("get ready\n");	
	//boot_param->status = 1;
	//asm volatile("" ::: "memory");
	
	/* get ready */
	boot_param->status = 2;
	
	kputs("change stack pointer\n");	

	asm volatile("" ::: "memory");
	asm volatile("movq %0, %%rsp" : : "r" (stack + sizeof(stack)));

	kputs("create page tables\n");	

	devices_pml1 = static_cast<uint64_t*>(alloc_pages(3));
	devices_pml2 = static_cast<uint64_t*>(alloc_pages(1));
	image_pml2 = static_cast<uint64_t*>(alloc_pages(1));
	pml2_tables = static_cast<uint64_t*>(alloc_pages(4));
	pml3_table = static_cast<uint64_t*>(alloc_pages(2));
	pml4_table = static_cast<uint64_t*>(alloc_pages(2));

	/* devices_pml1 */
	for(unsigned i = 0; i < 3*512; i++){
		devices_pml1[i] = INVALID;
	}

	/* devices_pml2 */
	devices_pml2[0] = PRESENT + WRITE + USER + ACCESSED + table_to_phys_addr(devices_pml1,0); // LAPIC
	devices_pml2[1] = PRESENT + WRITE + USER + ACCESSED + table_to_phys_addr(devices_pml1,1); // kernel stacks
	devices_pml2[2] = PRESENT + WRITE + USER + ACCESSED + table_to_phys_addr(devices_pml1,2); // kernel stacks
	for(unsigned i = 3; i < 512; i++){
		devices_pml1[i] = INVALID;
	}

	/* image_pml2 */
	for(unsigned i = 0; i < 1024; i++){
		image_pml2[i] = INVALID;	
	}	
	for(unsigned i = 0; i < 4; i++){
		image_pml2[512+0x1f4 + i] = PML2_BASE + i * PML2_PAGESIZE + x86_kernel_phys_base;	
	}	


	/* pml2_tables */
	for(unsigned i = 0; i < 2048; i++){
		pml2_tables[i] = PML2_BASE + i*PML2_PAGESIZE; 
	}

	/* pml3_table */
	pml3_table[0] = PML3_BASE + table_to_phys_addr(pml2_tables,0);
	pml3_table[1] = PML3_BASE + table_to_phys_addr(pml2_tables,1);
	pml3_table[2] = PML3_BASE + table_to_phys_addr(pml2_tables,2);
	pml3_table[3] = PML3_BASE + table_to_phys_addr(pml2_tables,3);
	pml3_table[4] = PML3_BASE + table_to_phys_addr(devices_pml2,0);
	for(unsigned i = 5; i < 1022; i++){
		pml3_table[i] = INVALID; 
	}
	pml3_table[1022] = PML3_BASE + table_to_phys_addr(image_pml2,0);
	pml3_table[1023] = PML3_BASE + table_to_phys_addr(image_pml2,1);
	

	/* pml4_table */
	pml4_table[0] = PML4_BASE + table_to_phys_addr(pml3_table,0);
	for(unsigned i = 1; i < 256; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[256] = PML4_BASE + table_to_phys_addr(pml3_table,0);
	for(unsigned i = 257; i < 511; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[511] = PML4_BASE + table_to_phys_addr(pml3_table,1);
	for(unsigned i = 512; i < 768; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[768] = PML4_BASE + table_to_phys_addr(pml3_table,0);
	for(unsigned i = 769; i < 1023; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[1023] = PML4_BASE + table_to_phys_addr(pml3_table,1);


	void* rq = alloc_pages(1); 
	void* wq = alloc_pages(1); 

	boot_param->mikc_queue_recv = virt_to_phys(wq);
	boot_param->mikc_queue_send = virt_to_phys(rq);
	

	kputs("while loop\n");	
	//while (1){
		//kputs("l");
		//int i,j;
		//for(i=0;i<255;i++)
		   //for(j=0;j<255;j++);
	//};
	while(1);
	/* never return */
}
} // boot
} // mythos
