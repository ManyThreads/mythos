#include "util/compiler.hh"
#include "boot/bootparam.h"
#include "boot/pagetables.hh"

#include <cstdint>
#include "util/compiler.hh"
#include "cpu/GdtAmd64.hh"
#include "cpu/IdtAmd64.hh"
#include "cpu/IrqHandler.hh"
#include "cpu/kernel_entry.hh"
#include "cpu/CoreLocal.hh"
#include "cpu/hwthreadid.hh"
#include "cpu/ctrlregs.hh"
#include "cpu/LAPIC.hh"
#include "cpu/idle.hh"
#include "cpu/hwthread_pause.hh"
#include "cpu/fpu.hh"
#include "boot/memory-layout.h"
#include "boot/DeployKernelSpace.hh"
#include "boot/DeployHWThread.hh"
#include "boot/cxx-globals.hh"
#include "boot/mlog.hh"
#include "boot/apboot.hh"
#include "boot/kmem.hh"
#include "async/Place.hh"
#include "boot/load_init.hh"
#include "objects/KernelMemory.hh"
#include "objects/StaticMemoryRegion.hh"
#include "objects/ISchedulable.hh"
#include "objects/SchedulingContext.hh"
#include "objects/InterruptControl.hh"
#include "boot/memory-root.hh"

extern void entry_bsp();

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
	else if (va >= MAP_ST_START){
		return va - MAP_ST_START;
	}else{
		return va;
	}
}

inline uint64_t table_to_phys_addr(uint64_t* t, uint64_t subtable) {
    return virt_to_phys(t + subtable * 512);
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

struct ihk_kmsg_buf *kmsg_buf;

void memcpy_ringbuf(char const * buf, int len) {
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

void kputs(char const *buf)
{
	size_t len = strlen(buf);
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

//void putHex(uint64_t ul){
	//kputs("ph");
	//if(ul > 0){
		//char str[20];
		//uint8_t i = 19;
		//str[i--] = 0;
		//str[i--] = ' ';
		
		//for(; ul > 0; ul = ul >> 4){
			//char c = ul % 16;
			//if(c < 10){
				//str[i--] = c + '0';
			//}else{
				//str[i--] = c - 10 + 'a';
			//}
		//}

		//str[i--] = 'x';
		//str[i--] = '0';

		//kputs(&str[i+1]);
	//}else{	
		//kputs("0x0 ");
	//}
//}

void putHex(uint64_t ul){
	char str[] = "0x0123456789abcdef ";
	
	for(char i = 16; i > 0; i--){
		char c = ul % 16;
		ul = ul >> 4;
		if(c < 10){
			str[i+1] = c + '0';
		}else{
			str[i+1] = c - 10 + 'a';
		}
	}
	kputs(str);
}

template<typename T>
void putHex(T* ul){
	putHex(reinterpret_cast<uint64_t>(ul));
}

void dumpTable(uint64_t* table){
	kputs("\ndumpTable:");
	putHex((uint64_t)table);
	table = (uint64_t*)((((uint64_t)table) >> 12) << 12);	
	if(table){
		kputs("\n");
		for(int i = 0; i < 512; i++){
			if(table[i]){
				putHex(i);
				kputs(" ");
				putHex(table[i]);
				kputs("\n");
			}else{
				//summarize invalid entries
			}
		}
	}else{
		kputs("nullptr :(\n");
	}
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

	kmsg_buf = (struct ihk_kmsg_buf*)phys_to_virt(boot_param->msg_buffer);

	putHex(_ap_trampoline);
	kputs("Hello from the other side!\n");	
	putHex(phys_address);
	putHex(boot_param->bootstrap_mem_end);
	putHex(boot_param->bootstrap_mem_end - phys_address);

	void* rq = alloc_pages(1); 
	void* wq = alloc_pages(1); 

	boot_param->mikc_queue_recv = virt_to_phys(wq);
	boot_param->mikc_queue_send = virt_to_phys(rq);

	kputs("get ready\n");	
	//boot_param->status = 1;
	//asm volatile("" ::: "memory");
	
	/* get ready */
	boot_param->status = 2;
	asm volatile("" ::: "memory");
	
	kputs("change stack pointer\n");	

	asm volatile("movq %0, %%rsp" : : "r" (stack + sizeof(stack)));

	kputs("create page tables\n");	

	devices_pml1 = static_cast<uint64_t*>(alloc_pages(3));
	devices_pml2 = static_cast<uint64_t*>(alloc_pages(1));
	image_pml2 = static_cast<uint64_t*>(alloc_pages(2));
	pml2_tables = static_cast<uint64_t*>(alloc_pages(4));
	pml3_table = static_cast<uint64_t*>(alloc_pages(2));
	pml4_table = static_cast<uint64_t*>(alloc_pages(2));

	kputs("\n devices_pml1: ");	
	putHex(devices_pml1);
	putHex(virt_to_phys(devices_pml1));
	kputs("\n devices_pml2: ");	
	putHex(devices_pml2);
	putHex(virt_to_phys(devices_pml2));
	kputs("\n image_pml2: ");	
	putHex(image_pml2);
	putHex(virt_to_phys(image_pml2));
	kputs("\n pml2_tables: ");	
	putHex(pml2_tables);
	putHex(virt_to_phys(pml2_tables));
	kputs("\n pml3_table: ");	
	putHex(pml3_table);
	putHex(virt_to_phys(pml3_table));
	kputs("\n pml4_table: ");	
	putHex(pml4_table);
	putHex(virt_to_phys(pml4_table));

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
		if(phys_address + i * PML2_PAGESIZE < boot_param->bootstrap_mem_end){
			pml2_tables[i] = PML2_BASE + phys_address + i*PML2_PAGESIZE; 
		}else{
			pml2_tables[i] = INVALID; 
		}
	}

	/* pml3_table */
	pml3_table[0] = PML3_BASE + table_to_phys_addr(pml2_tables,0);
	pml3_table[1] = PML3_BASE + table_to_phys_addr(pml2_tables,1);
	pml3_table[2] = PML3_BASE + table_to_phys_addr(pml2_tables,2);
	pml3_table[3] = PML3_BASE + table_to_phys_addr(pml2_tables,3);
	pml3_table[4] = PML3_BASE + table_to_phys_addr(devices_pml2,0);
	for(unsigned i = 5; i < 512; i++){
		pml3_table[i] = INVALID; 
	}
	// second table
	for(unsigned i = 512; i < 1022; i++){
		pml3_table[i] = INVALID; 
	}
	pml3_table[1022] = PML3_BASE + table_to_phys_addr(image_pml2,0);
	pml3_table[1023] = PML3_BASE + table_to_phys_addr(image_pml2,1);
	

	/* pml4_table */
	uint64_t* cr3;
	asm volatile ("mov %%cr3, %0":"=r" (cr3));
	//putHex((uint64_t)cr3);
	pml4_table[0] = cr3[0]; 
	pml4_table[1] = cr3[1]; 
	//pml4_table[0] = PML4_BASE + table_to_phys_addr(pml3_table,0);
	for(unsigned i = 2; i < 256; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[256] = cr3[256];
	pml4_table[257] = cr3[257];
	pml4_table[258] = PML4_BASE + table_to_phys_addr(pml3_table,0);
	for(unsigned i = 259; i < 511; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[511] = /*cr3[511];*/PML4_BASE + table_to_phys_addr(pml3_table,1);

	//second table (user land)
	for(unsigned i = 512; i < 768; i++){
		pml4_table[i] = INVALID; 
	}

	pml4_table[512+0] = cr3[0]; 
	pml4_table[512+1] = cr3[1]; 
	pml4_table[512+256] = cr3[256];
	pml4_table[512+257] = cr3[257];
	pml4_table[512+258] = PML4_BASE + table_to_phys_addr(pml3_table,0);
	for(unsigned i = 771; i < 1023; i++){
		pml4_table[i] = INVALID; 
	}
	pml4_table[1023] = /*cr3[511];*/ PML4_BASE + table_to_phys_addr(pml3_table,1);

	dumpTable(cr3);
	dumpTable(pml4_table);


	dumpTable((uint64_t*)cr3[511]);
	dumpTable((uint64_t*)pml4_table[511]);

	dumpTable((uint64_t*)((uint64_t*)(cr3[511] & 0xFFFFFFFFFFFFFF00))[511]);
	dumpTable((uint64_t*)((uint64_t*)(pml4_table[511] & 0xFFFFFFFFFFFFFF00))[511]);

	asm volatile("movq %0, %%cr3" : : "r" (table_to_phys_addr(pml4_table,0)));
	//asm volatile("movq %0, %%cr3" : : "r" (cr3));

	/* entry_bsp */	
	entry_bsp();

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
