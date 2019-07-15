/* mikc.c COPYRIGHT FUJITSU LIMITED 2015-2016 */
/** 
 * \file host/linux/mikc.c
 *
 * \brief IHK-Host: Establishes a master channel, and also defines functions
 *        used in IHK-IKC.
 *
 * \author Taku Shimosawa  <shimosawa@is.s.u-tokyo.ac.jp> \par
 * Copyright (C) 2011 - 2012  Taku Shimosawa
 * 
 */
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/file.h>
#include <asm/spinlock.h>
#include <ihk/ihk_host_user.h>
#include <ihk/ihk_host_driver.h>
#include <ihk/misc/debug.h>
#include <ikc/queue.h>
#include <ikc/msg.h>
#include "host_linux.h"

#define DEBUG_IKC

#ifdef DEBUG_IKC
#define	dkprintf(...) kprintf(__VA_ARGS__)
#define	ekprintf(...) kprintf(__VA_ARGS__)
#else
#define dkprintf(...) do { if (0) printk(__VA_ARGS__); } while (0)
#define	ekprintf(...) printk(__VA_ARGS__)
#endif

#define dprintf printk

static int arch_master_handler(struct ihk_ikc_channel_desc *c,
                               void *__packet, void *__os);
/**
 * \brief Core function of initialization of a master channel.
 * It waits for the kernel to become ready, maps the queues,
 * and allocates a channel descriptor strcuture for the master channel.
 */
struct ihk_ikc_channel_desc *ihk_host_ikc_init_first(ihk_os_t ihk_os,
                                                     ihk_ikc_ph_t handler)
{
	struct ihk_host_linux_os_data *os = ihk_os;
	unsigned long r, w, rp, wp, rsz, wsz;
	struct ihk_ikc_queue_head *rq, *wq;
	struct ihk_ikc_channel_desc *c;

	ihk_ikc_system_init(ihk_os);
	os->ikc_initialized = 1;

	if (ihk_os_wait_for_status(ihk_os, IHK_OS_STATUS_READY, 0, 200) == 0) {
		/* XXX: 
		 * We assume this address is remote, 
		 * but the local is possible... */
		dprintf("OS is now marked ready.\n");

		if(ihk_os_get_special_address(ihk_os, IHK_SPADDR_MIKC_QUEUE_RECV,
		                           &r, &rsz)) return 0;

		if(ihk_os_get_special_address(ihk_os, IHK_SPADDR_MIKC_QUEUE_SEND,
		                           &w, &wsz)) return 0;

		dprintf("MIKC rq: 0x%lX, wq: 0x%lX\n", r, w);

		rp = ihk_device_map_memory(os->dev_data, r, rsz);
		wp = ihk_device_map_memory(os->dev_data, w, wsz);
		
		rq = ihk_device_map_virtual(os->dev_data, rp, rsz, NULL, 0);
		wq = ihk_device_map_virtual(os->dev_data, wp, wsz, NULL, 0);
		
		c = kzalloc(sizeof(struct ihk_ikc_channel_desc)
		            + sizeof(struct ihk_ikc_master_packet), GFP_KERNEL);
		ihk_ikc_init_desc(c, ihk_os, 0, rq, wq,
		                  ihk_ikc_master_channel_packet_handler, c);

		ihk_ikc_channel_set_cpu(c, 0);

		c->recv.qphys = rp;
		c->send.qphys = wp;
		c->recv.qrphys = r;
		c->send.qrphys = w;
		/*
		 * ihk_ikc_interrupt_handler() on the LWK now iterates the channel
		 * until all packets are purged. This makes the notification IRQ
		 * on master channel unnecessary.
		 */
		//c->flag |= IKC_FLAG_NO_COPY;

		dprintf("c->remote_os = %p\n", c->remote_os);
		os->packet_handler = handler;

		return c;
	} else {
		printk("IHK: OS does not become ready, kernel msg:\n");
		ihk_host_print_os_kmsg(ihk_os);
		return NULL;
	}
}

/** \brief Initializes a master channel */
int ihk_ikc_master_init(ihk_os_t __os)
{
	struct ihk_host_linux_os_data *os = __os;
	struct ihk_ikc_master_packet packet;

	dprintf("ikc_master_init\n");

	if (!os) {
		return -EINVAL;
	}

	os->mchannel = 
		ihk_host_ikc_init_first(os, arch_master_handler);
	dprintf("os(%p)->mchannel = %p\n", os, os->mchannel);
	if (!os->mchannel) {
		return -EINVAL;
	} else {
		/*ihk_ikc_enable_channel(os->mchannel);*/
		
		/*dprintf("ikc_master_init done.\n");*/

		/*[> ack send <]*/
		/*packet.msg = IHK_IKC_MASTER_MSG_INIT_ACK;*/
		/*ihk_ikc_send(os->mchannel, &packet, 0);*/

		return 0;
	}
}

/** \brief Called when the kernel is going to shutdown. It finalizes
 * the master channel. */
void ikc_master_finalize(ihk_os_t __os)
{
	struct ihk_host_linux_os_data *os = __os;

	dprint_func_enter;

	if (!os->ikc_initialized) {
		return;
	}

	if (os->mchannel) {
		ihk_ikc_destroy_channel(os->mchannel);
	}
	ihk_ikc_system_exit(os);

	os->ikc_initialized = 0;
}

/** \brief Get the list of channel (called from IHK-IKC) */
struct list_head *ihk_os_get_ikc_channel_list(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &os->ikc_channels;
}

/** \brief Get the lock for the channel list (called from IHK-IKC) */
spinlock_t *ihk_os_get_ikc_channel_lock(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &os->ikc_channel_lock;
}

/** \brief Get the IKC regular channel (called from IHK-IKC) */
struct ihk_ikc_channel_desc *ihk_os_get_regular_channel(ihk_os_t ihk_os, int cpu)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return os->regular_channels[cpu];
}

/** \brief Set the IKC regular channel (called from IHK-IKC) */
void ihk_os_set_regular_channel(ihk_os_t ihk_os, struct ihk_ikc_channel_desc *c, int cpu)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	if (cpu < 0 || cpu > num_possible_cpus()) {
		dprintf("%s: WARNING: invalid CPU number: %d\n", __FUNCTION__, cpu);
		return;
	}
	os->regular_channels[cpu] = c;
}

/** \brief Get the interrupt handler of the IKC (called from IHK-IKC) */
struct ihk_host_interrupt_handler *ihk_host_os_get_ikc_handler(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &os->ikc_handler;
}

/** \brief Issue an interrupt to the receiver of the channel
 *  (called from IHK-IKC) */
int ihk_ikc_send_interrupt(struct ihk_ikc_channel_desc *channel)
{
	return ihk_os_issue_interrupt(channel->remote_os, 
	                              channel->send.queue->read_cpu,
/* POSTK_DEBUG_ARCH_DEP_10 */
#if defined(__aarch64__)
	                              0x01);
#else
	                              0xd1);
#endif
}

/** \brief Get the lock for the list of listeners (called from IHK-IKC) */
ihk_spinlock_t *ihk_ikc_get_listener_lock(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &os->listener_lock;
}

/** \brief Get the pointer of the specified port in the list of listeners
 * (called from IHK-IKC) */
struct ihk_ikc_listen_param **ihk_ikc_get_listener_entry(ihk_os_t ihk_os,
                                                         int port)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &(os->listeners[port]);
}

/** \brief Invokes the master packet handler of the specified kernel
 * (called from IHK-IKC) */
int ihk_ikc_call_master_packet_handler(ihk_os_t ihk_os,
                                       struct ihk_ikc_channel_desc *c,
                                       void *packet)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	if (os->packet_handler) {
		return os->packet_handler(c, packet, os);
	}
	return 0;
}

/** \brief Invokes the master packet handler of the specified kernel
 * (called from IHK-IKC) */
static int arch_master_handler(struct ihk_ikc_channel_desc *c,
                               void *__packet, void *__os)
{
	struct ihk_ikc_master_packet *packet = __packet;

	/* TODO */
	printk("Unhandled master packet! : %x\n", packet->msg);
	return 0;
}

/** \brief Get the wait list for the master channel (Called from IHK-IKC) */
struct list_head *ihk_ikc_get_master_wait_list(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &os->wait_list;
}
/** \brief Get the lock for the wait list for the master channel (called from
 *         IHK-IKC) */
ihk_spinlock_t *ihk_ikc_get_master_wait_lock(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;

	return &os->wait_lock;
}

/** \brief Get the master channel of the specified kernel
 *         (Called from IHK-IKC) */
struct ihk_ikc_channel_desc *ihk_os_get_master_channel(ihk_os_t __os)
{
	struct ihk_host_linux_os_data *os = __os;

	return os->mchannel;
}

/** \brief Generate a unique ID for a channel
 *         (Called from IHK-IKC) */
int ihk_os_get_unique_channel_id(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;
	
	return atomic_inc_return(&os->channel_id);
}

/** \brief Initialize the work thread structure
 *         (Called from IHK-IKC) */
void ihk_ikc_linux_init_work_data(ihk_os_t ihk_os,
                                  void (*f)(struct work_struct *))
{
	struct ihk_host_linux_os_data *os = ihk_os;

	os->work_function = f;
}

struct ikc_work_struct {
	struct work_struct work;
	ihk_os_t os;
};

/** \brief Schedule the work thread (Called from IHK-IKC) */
void ihk_ikc_linux_schedule_work(ihk_os_t ihk_os)
{
	struct ihk_host_linux_os_data *os = ihk_os;
	struct ikc_work_struct *work;

	work = kmalloc(sizeof(struct ikc_work_struct), GFP_ATOMIC);
	if (work == NULL) {
		return;
	}
	INIT_WORK(&work->work, os->work_function);
	work->os = ihk_os;
	schedule_work_on(smp_processor_id(), &work->work);
}

/** \brief Get ihk_os_t from the work struct */
ihk_os_t ihk_ikc_linux_get_os_from_work(struct work_struct *work)
{
	struct ikc_work_struct *ikc_work = (struct ikc_work_struct *)work;

	return ikc_work->os;
}
