#ifndef HEADER_BUILTIN_BOOTPARAM_H
#define HEADER_BUILTIN_BOOTPARAM_H

#define SMP_MAX_CPUS 512

#define __NCOREBITS  (sizeof(long) * 8)   /* bits per mask */
#define CORE_SET(n, p) \
	((p).set[(n)/__NCOREBITS] |= ((long)1 << ((n) % __NCOREBITS)))
#define CORE_CLR(n, p) \
	((p).set[(n)/__NCOREBITS] &= ~((long)1 << ((n) % __NCOREBITS)))
#define CORE_ISSET(n, p) \
	(((p).set[(n)/__NCOREBITS] & ((long)1 << ((n) % __NCOREBITS)))?1:0)
#define CORE_ZERO(p)      memset(&(p).set, 0, sizeof((p).set))

//struct smp_coreset {
	//unsigned long set[SMP_MAX_CPUS / __NCOREBITS];
//};

//static inline int CORE_ISSET_ANY(struct smp_coreset *p)
//{
	//int     i;

	//for(i = 0; i < SMP_MAX_CPUS / __NCOREBITS; i++)
		//if(p -> set[i])
			//return 1;
	//return 0;
//}

struct ihk_smp_boot_param_cpu {
	int numa_id;
	int hw_id;
	int linux_cpu_id;
	int ikc_cpu;
};

struct ihk_smp_boot_param_memory_chunk {
	unsigned long start, end;
	int numa_id;
};

#define IHK_SMP_MEMORY_TYPE_DRAM          0x01
#define IHK_SMP_MEMORY_TYPE_HBM           0x02

struct ihk_smp_boot_param_numa_node {
	int type;
	int linux_numa_id;
};

//struct ihk_dump_page {
	//unsigned long start;
	//unsigned long map_count;
	//unsigned long map[0];
//};

#define IHK_DUMP_PAGE_SET_INCOMPLETE 0
#define IHK_DUMP_PAGE_SET_COMPLETED  1

struct ihk_dump_page_set {
	volatile unsigned int completion_flag;
	unsigned int count;
	unsigned long page_size;
	unsigned long phy_page;
};

/*
 * smp_boot_param holds various boot time arguments.
 * The layout in the memory is the following:
 * [[struct smp_boot_param]
 * [struct ihk_smp_boot_param_cpu] ... [struct ihk_smp_boot_param_cpu]
 * [struct ihk_smp_boot_param_numa_node] ...
 * [struct ihk_smp_boot_param_numa_node]
 * [struct ihk_smp_boot_param_memory_chunk] ...
 * [struct ihk_smp_boot_param_memory_chunk]],
 * where the number of CPUs, the number of numa nodes and
 * the number of memory ranges are determined by the nr_cpus,
 * nr_numa_nodes and nr_memory_chunks fields, respectively.
 */
struct smp_boot_param {
	/*
	 * [start, end] covers all assigned ranges, including holes
	 * in between so that a straight mapping can be set up at boot time,
	 * but actual valid memory ranges are described in the
	 * ihk_smp_boot_param_memory_chunk structures.
	 */
	unsigned long start, end;
	volatile unsigned long status;
	/* Size of this structure (including all the postix data) */
	int param_size;

    /* End address of the memory chunk on which kernel sections 
       are loaded, used for boundary check in early_alloc_pages(). */
	unsigned long bootstrap_mem_end;

	unsigned long msg_buffer; /* Physical address */
	unsigned long msg_buffer_size;
	unsigned long mikc_queue_recv, mikc_queue_send;

	unsigned long monitor;
	unsigned long monitor_size;

	unsigned long rusage;
	unsigned long rusage_size;

	unsigned long nmi_mode_addr;
	unsigned long mckernel_do_futex;
	unsigned long linux_kernel_pgt_phys;

	unsigned long dma_address;
	unsigned long ident_table;
	unsigned long ns_per_tsc;
	unsigned long boot_tsc;
	unsigned long boot_sec;
	unsigned long boot_nsec;
//#ifdef IHK_IKC_USE_LINUX_WORK_IRQ
	//void *ihk_ikc_cpu_raised_list[SMP_MAX_CPUS];
	//void *ikc_irq_work_func;
//#endif // IHK_IKC_USE_LINUX_WORK_IRQ
	unsigned int ihk_ikc_irq;
	unsigned int ihk_ikc_irq_apicids[SMP_MAX_CPUS];
	char kernel_args[256];
	int nr_linux_cpus;
	int nr_cpus;
	int nr_numa_nodes;
	int nr_memory_chunks;
	int osnum;
	unsigned int dump_level;
	int linux_default_huge_page_shift;
	struct ihk_dump_page_set dump_page_set;
};

extern struct smp_boot_param *boot_param;

#endif
