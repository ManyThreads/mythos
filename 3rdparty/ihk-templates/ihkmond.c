/**
 * \file ihkmond.c
 *  License details are found in the file LICENSE.
 * \brief
 *  Sequence hungup detector and kmsg taker with the coordination with creation/destruction events of McKernel
 * \author Masamichi Takagi  <masamichi.takagi@riken.jp> \par
 * 	Copyright (C) 2017  Masamichi Takagi
 **/

/**  
 *  Kill (and restart) ihkmond when destroying /dev/mcosX without notifying mcudevd by eventfd
 *  to clean-up fd and eventfd
 **/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <syslog.h>
#include <libudev.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <config.h>
#include <ihk/ihklib.h>
#include <ihk/ihklib_private.h>
#include <ihk/ihk_host_user.h>
#include <sys/timerfd.h>

#define DEBUG

#ifdef DEBUG
#define	dprintf(...)											\
	do {														\
		char msg[1024];											\
		sprintf(msg, __VA_ARGS__);								\
		fprintf(stderr, "%s,%s", __FUNCTION__, msg);			\
	} while (0);
#define	eprintf(...)											\
	do {														\
		char msg[1024];											\
		sprintf(msg, __VA_ARGS__);								\
		fprintf(stderr, "%s,%s", __FUNCTION__, msg);			\
	} while (0);
#else
#define dprintf(...) do {  } while (0)
#define eprintf(...) do {										\
		char msg[1024];											\
		sprintf(msg, __VA_ARGS__);								\
		fprintf(stderr, "%s,%s", __FUNCTION__, msg);			\
  } while (0)
#endif

#define CHKANDJUMP(cond, err, ...)								\
	do {																\
		if(cond) {														\
			eprintf(__VA_ARGS__);										\
			ret = err;													\
			goto out;												\
		}																\
	} while(0)

#define IHKMOND_SIZE_FILEBUF_SLOT (1 * (1ULL << 20))
#define IHKMOND_NUM_FILEBUF_SLOTS 64
#define IHKMOND_TMP "/tmp/ihkmond"

struct thr_args {
	pthread_t thread;
	pthread_mutex_t lock;
	int mcos_added;
	pthread_cond_t cond_mcos_added; /* Cond for add event */
	int ret; /* Return value */

	int dev_index; /* device index */
	int os_index; /* OS index */
	int facility; /* facility field for syslog */
	char* logid; /* id field for syslog */
	int interval; /* Polling interval */

	int evfd_mcos_removed; /* Remove event */
};

struct facility_list {
		char name[12];
		int code;
};

struct facility_list facility_list[8] = {
	{"LOG_LOCAL0", LOG_LOCAL0},
	{"LOG_LOCAL1", LOG_LOCAL1},
	{"LOG_LOCAL2", LOG_LOCAL2},
	{"LOG_LOCAL3", LOG_LOCAL3},
	{"LOG_LOCAL4", LOG_LOCAL4},
	{"LOG_LOCAL5", LOG_LOCAL5},
	{"LOG_LOCAL6", LOG_LOCAL6},
	{"LOG_LOCAL7", LOG_LOCAL7}
};

const struct option longopt[] = {
	{"help", no_argument, NULL, '?'},
	{NULL, 0, NULL, 0}
};

static ssize_t reap_event(int evfd) {
	uint64_t counter;
	ssize_t ret = read(evfd, &counter, sizeof(counter));
	CHKANDJUMP(ret == 0, -1, "EOF detected\n");
	CHKANDJUMP(ret == -1, -1, "error: %s\n", strerror(errno));
 out:
	return ret;
}

static void* detect_hungup(void* _arg) {
	struct thr_args *arg = (struct thr_args *)_arg;
	int osfd = -1, epfd = -1;
	struct epoll_event event;
	struct epoll_event events[1];
	int ret = 0, ret_lib;
	int i;
	char fn[32];
	struct stat st;

	epfd = epoll_create(1);
	CHKANDJUMP(epfd == -1, 255, "epoll_create failed\n");
	
	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = arg->evfd_mcos_removed;
	ret_lib = epoll_ctl(epfd, EPOLL_CTL_ADD, arg->evfd_mcos_removed, &event);
	CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");

 wait_for_add:
	dprintf("wait_for_add,os_index=%d\n", arg->os_index);

	ret_lib = pthread_mutex_lock(&arg->lock);
	CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_lock failed\n");
	while (arg->mcos_added == 0) {
		ret_lib = pthread_cond_wait(&arg->cond_mcos_added, &arg->lock);
	}
	arg->mcos_added = 0;
	pthread_mutex_unlock(&arg->lock);

	i = 0;
	snprintf(fn, sizeof(fn), "/dev/mcos%d", arg->os_index);
	while (stat(fn, &st) == -1) {
		CHKANDJUMP(errno != ENOENT, -errno,
			"/dev/mcosX access failed\n");
		usleep(200);
		i++;

		/* about 10s timeout check */
		CHKANDJUMP(50 * 1000 < i, -ETIMEDOUT,
			"/dev/mcosX create timeout\n");
	}

 next:
	dprintf("next\n");

	osfd = ihklib_os_open(arg->os_index);
	CHKANDJUMP(osfd < 0, -errno, "ihklib_os_open failed\n");

	ret_lib = ioctl(osfd, IHK_OS_DETECT_HUNGUP);
	if(ret_lib == -1) {
		if(errno == EAGAIN) { /* OS is booting */
			dprintf("%s: ioctl IHK_OS_DETECT_HUNGUP returned EAGAIN\n", __FUNCTION__);
		} else {
			dprintf("%s: ioctl IHK_OS_DETECT_HUNGUP returned %s\n", __FUNCTION__, strerror(errno));
		}
	} else {
		dprintf("%s: ioctl IHK_OS_DETECT_HUNGUP returned %d\n", __FUNCTION__, ret_lib);
	}
	close(osfd);
	osfd = -1;

	do {
		int nfd = epoll_wait(epfd, events, 1, arg->interval * 1000);
		if (nfd < 0 && errno == EINTR)
			continue;
		CHKANDJUMP(nfd < 0, -EINVAL, "epoll_wait failed\n");
		if (nfd == 0) {
			goto next;
		}
		for (i = 0; i < nfd; i++) {
			if (events[i].data.fd == arg->evfd_mcos_removed) {
				dprintf("mcos remove event detected\n");
				goto wait_for_add;
			}
		}
	} while (1);

out:
	if (osfd >= 0) {
		close(osfd);
	}
	if (epfd >= 0) {
		close(epfd);
	}
	arg->ret = ret;
	return NULL;
}

static int ihkmond_device_open(int dev_index) {
	int i, devfd;
	for (i = 0; i < 4; i++) {
		devfd = ihklib_device_open(dev_index);
		if (devfd >= 0) {
			break;
		}
		usleep(200);
	}
	if (i == 4) {
		eprintf("Warning: ihklib_device_open failed %d times\n", i);
	}
	return devfd;
}

static int fwrite_kmsg(int dev_index, void* handle, int os_index, FILE **fps, int *sizes, int *prod, int shift) {
	int ret = 0, ret_lib;
	int devfd = -1;
	ssize_t nread;
	char buf[IHK_KMSG_SIZE];
	char fn[256];
	int next_slot = 0;
	struct ihk_device_read_kmsg_buf_desc desc = { .handle = handle, .shift = shift, .buf = buf };
	
	devfd = ihkmond_device_open(dev_index);
	CHKANDJUMP(devfd < 0, -errno, "ihkmond_device_open returned %d\n", -errno);

	nread = ioctl(devfd, IHK_DEVICE_READ_KMSG_BUF, (unsigned long)&desc);
	CHKANDJUMP(nread < 0 || nread > IHK_KMSG_SIZE, nread, "ioctl failed\n");
	if (nread == 0) {
		dprintf("nread is zero\n");
		goto out;
	}
	close(devfd);
	devfd = -1;

	if (sizes[*prod] + nread > IHKMOND_SIZE_FILEBUF_SLOT) {
		*prod = (*prod + 1) % IHKMOND_NUM_FILEBUF_SLOTS;
		next_slot = 1;
	}

	if (next_slot || fps[*prod] == NULL) {
		if (fps[*prod] == NULL) {
			sprintf(fn, IHKMOND_TMP);
			ret_lib = mkdir(fn, 0755);
			CHKANDJUMP(ret_lib != 0 && errno != EEXIST, -errno, "mkdir failed\n");

			sprintf(fn, IHKMOND_TMP "/mcos%d", os_index);
			ret_lib = mkdir(fn, 0755);
			CHKANDJUMP(ret_lib != 0 && errno != EEXIST, -errno, "mkdir failed\n");
		} else {
			fclose(fps[*prod]);
			fps[*prod] = NULL;
		}

		sprintf(fn, IHKMOND_TMP "/mcos%d/kmsg%d", os_index, *prod);
		fps[*prod] = fopen(fn, "w+");
		CHKANDJUMP(fps[*prod] == NULL, -EINVAL, "fopen failed\n"); 
		sizes[*prod] = 0;
		dprintf("fn=%s\n", fn);
	}

	ret = fwrite(buf, 1, nread, fps[*prod]); 
	sizes[*prod] += nread;
	/*dprintf("fwrite returned %d\n", ret);*/
 out:
	if (devfd >= 0) {
		close(devfd);
	}
	return ret;
}

static ssize_t syslog_kmsg(FILE **fps, int prod) {
	int ret = 0;
	char *buf = NULL;
	char *cur;
	char *token;
	int cons;
	int i;

	buf = malloc(IHKMOND_SIZE_FILEBUF_SLOT + 1);
	CHKANDJUMP(buf == NULL, -ENOMEM, "malloc failed");

	for(i = IHKMOND_NUM_FILEBUF_SLOTS - 1; i >= 0; i--) {
		if (fps[i] == NULL) {
			cons = 0;
			goto no_wrap_around;
		}
	}
	cons = (prod + 1) % IHKMOND_NUM_FILEBUF_SLOTS;
 no_wrap_around:
	dprintf("%s: prod=%d,cons=%d\n", __FUNCTION__, prod, cons);

	for(i = 0; i < IHKMOND_NUM_FILEBUF_SLOTS && fps[cons] != NULL; i++, cons = (cons + 1) % IHKMOND_NUM_FILEBUF_SLOTS) {
		dprintf("cons=%d\n", cons);
		rewind(fps[cons]);
		cur = buf;
		while((ret = fread(cur, 1, IHKMOND_SIZE_FILEBUF_SLOT - (cur - buf) - 1, fps[cons])), ret > 0) {
			cur += ret;
		}
		CHKANDJUMP(ferror(fps[cons]), -EINVAL, "ferror()\n");
		*cur = 0;
		dprintf("total=%ld\n", (unsigned long)(cur - buf));

		/* Mark as consumed for duplicated call to this function */
		fclose(fps[cons]);
		fps[cons] = NULL;

		cur = buf;
		token = strsep(&cur, "\n");
		while (token != NULL) {
			if(*token == 0) {
				goto empty_token;
			}
			dprintf("token=%s\n", token);
			syslog(LOG_INFO, "%s", token);
			usleep(200); /* Prevent syslog from dropping messages */
		empty_token:
			token = strsep(&cur, "\n");
		}

	}
	dprintf("%d slot(s) transferred\n", i);

 out:
	if (buf) {
		free(buf);
	}
	return ret;
}

static void* redirect_kmsg(void* _arg) {
	struct thr_args *arg = (struct thr_args *)_arg;
	int devfd = -1, evfd_kmsg = -1, evfd_status = -1, epfd = -1, timerfd = -1;
	struct epoll_event event;
	struct epoll_event events[20];
	int ret = 0, ret_lib;
	int i;
	FILE* fps[IHKMOND_NUM_FILEBUF_SLOTS];
	int sizes[IHKMOND_NUM_FILEBUF_SLOTS];
	int prod = 0; /* Producer pointer */
	struct ihk_device_get_kmsg_buf_desc desc_get;
	struct itimerspec its;

	memset(fps, 0, IHKMOND_NUM_FILEBUF_SLOTS * sizeof(FILE *));
	memset(sizes, 0, IHKMOND_NUM_FILEBUF_SLOTS * sizeof(int));

	openlog(arg->logid, LOG_PID, arg->facility);

	epfd = epoll_create(1);
	CHKANDJUMP(epfd == -1, 255, "epoll_create failed\n");
	
	/* mcos remove event live longer than /dev/mcosX itself */
	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = arg->evfd_mcos_removed;
	ret_lib = epoll_ctl(epfd, EPOLL_CTL_ADD, arg->evfd_mcos_removed, &event);
	CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");

 wait_for_mcos:
	dprintf("wait for mcos add, dev_index=%d, os_index=%d\n", arg->dev_index, arg->os_index);

	/* Wait for add /dev/mcosX event */
	ret_lib = pthread_mutex_lock(&arg->lock);
	CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_lock failed\n");
	while (arg->mcos_added == 0) {
		ret_lib = pthread_cond_wait(&arg->cond_mcos_added, &arg->lock);
	}
	arg->mcos_added = 0;
	pthread_mutex_unlock(&arg->lock);
	
	dprintf("mcos add detected\n");

	/* Get (i.e. ref) kmsg_buf */
	devfd = ihkmond_device_open(arg->dev_index); 
	CHKANDJUMP(devfd < 0, -errno, "ihkmond_device_open returned %d\n", devfd);
	
	memset(&desc_get, 0, sizeof(desc_get));
	desc_get.os_index = arg->os_index;
	ret_lib = ioctl(devfd, IHK_DEVICE_GET_KMSG_BUF, &desc_get);
	CHKANDJUMP(ret_lib < 0, ret_lib, "IHK_DEVICE_GET_KMSG_BUF returned %d\n", ret_lib);
	dprintf("IHK_DEVICE_GET_KMSG_BUF returned %p\n", desc_get.handle);

	close(devfd);
	devfd = -1;
	
	/* Get notification when the amount of kmsg exceeds a threshold */
	evfd_kmsg = ihk_os_get_eventfd(arg->os_index, IHK_OS_EVENTFD_TYPE_KMSG);
	CHKANDJUMP(evfd_kmsg < 0, -EINVAL, "ihk_os_get_eventfd\n");

	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = evfd_kmsg;
	ret_lib = epoll_ctl(epfd, EPOLL_CTL_ADD, evfd_kmsg, &event);
	CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");

	/* periodically trigger kmsg-read */
	timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
	CHKANDJUMP(timerfd < 0, -EINVAL, "timerfd\n");

	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = timerfd;
	ret_lib = epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &event);
	CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");

	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = 0;

	its.it_interval.tv_sec = 2; // Every 2 seconds interval
	its.it_interval.tv_nsec = 0;

	timerfd_settime(timerfd, 0, &its, NULL);

	/* Get notification when LWK panics or gets hungup */
	evfd_status = ihk_os_get_eventfd(arg->os_index, IHK_OS_EVENTFD_TYPE_STATUS);
	CHKANDJUMP(evfd_status < 0, -EINVAL, "ihk_os_get_eventfd\n");

	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = evfd_status;
	ret_lib = epoll_ctl(epfd, EPOLL_CTL_ADD, evfd_status, &event);
	CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");

	do {
		int nfd = epoll_wait(epfd, events, 20, -1);
		if (nfd < 0 && errno == EINTR)
			continue;
		CHKANDJUMP(nfd < 0, -EINVAL, "epoll_wait failed\n");
		for (i = 0; i < nfd; i++) {
			if (events[i].data.fd == timerfd) {
				reap_event(events[i].data.fd);
				/*dprintf("timer event detected\n");*/
				ret_lib = fwrite_kmsg(arg->dev_index, desc_get.handle, arg->os_index, fps, sizes, &prod, 1);
				/*CHKANDJUMP(ret_lib < 0, -EINVAL, "fwrite_kmsg returned %d\n", ret_lib);*/
			} else if (events[i].data.fd == evfd_kmsg) {
				reap_event(events[i].data.fd);
				dprintf("kmsg event detected\n");
				ret_lib = fwrite_kmsg(arg->dev_index, desc_get.handle, arg->os_index, fps, sizes, &prod, 1);
				CHKANDJUMP(ret_lib < 0, -EINVAL, "fwrite_kmsg returned %d\n", ret_lib);
			} else if (events[i].data.fd == evfd_status) {
				reap_event(events[i].data.fd);
				dprintf("LWK status event detected\n");
				ret_lib = fwrite_kmsg(arg->dev_index, desc_get.handle, arg->os_index, fps, sizes, &prod, 1);
				CHKANDJUMP(ret_lib < 0, -EINVAL, "fwrite_kmsg returned %d\n", ret_lib);

				ret_lib = syslog_kmsg(fps, prod);
				CHKANDJUMP(ret_lib < 0, ret_lib, "syslog_kmsg returned %d\n", ret_lib);
			} else if (events[i].data.fd == arg->evfd_mcos_removed) {
				reap_event(events[i].data.fd);
				dprintf("mcos remove event detected\n");
				ret_lib = fwrite_kmsg(arg->dev_index, desc_get.handle, arg->os_index, fps, sizes, &prod, 1);
				CHKANDJUMP(ret_lib < 0, -EINVAL, "fwrite_kmsg returned %d\n", ret_lib);

				ret_lib = syslog_kmsg(fps, prod);
				CHKANDJUMP(ret_lib < 0, ret_lib, "syslog_kmsg returned %d\n", ret_lib);
				dprintf("after syslog_kmsg for destroy\n");
#if 1
				/* Release (i.e. unref) kmsg_buf */
				devfd = ihkmond_device_open(arg->dev_index); 
				CHKANDJUMP(devfd < 0, -errno, "ihkmond_device_open returned %d\n", devfd);
				ret_lib = ioctl(devfd, IHK_DEVICE_RELEASE_KMSG_BUF, desc_get.handle);
				CHKANDJUMP(ret_lib != 0, ret_lib, "IHK_DEVICE_RELEASE_KMSG_BUF failed\n");
				close(devfd);
				devfd = -1;
#endif
				ret_lib = epoll_ctl(epfd, EPOLL_CTL_DEL, evfd_kmsg, &event);
				CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");
				close(evfd_kmsg);
				evfd_kmsg = -1;

				ret_lib = epoll_ctl(epfd, EPOLL_CTL_DEL, evfd_status, &event);
				CHKANDJUMP(ret_lib != 0, -EINVAL, "epoll_ctl failed\n");
				close(evfd_status);
				evfd_status = -1;

				for (i = 0; i < IHKMOND_NUM_FILEBUF_SLOTS; i++) {
					if(fps[i] != NULL) {
						fclose(fps[i]);
						fps[i] = NULL;
#if 0
						char fn[256];
						sprintf(fn, IHKMOND_TMP "/mcos%d/kmsg%d", arg->os_index, i);
						ret_lib = unlink(fn);
						CHKANDJUMP(ret_lib != 0, -EINVAL, "unlink failed\n");
						sprintf(fn, IHKMOND_TMP "/mcos%d", arg->os_index);
						ret_lib = rmdir(fn);
						CHKANDJUMP(ret_lib != 0, -EINVAL, "rmdir failed\n");
						sprintf(fn, IHKMOND_TMP);
						ret_lib = rmdir(fn);
						CHKANDJUMP(ret_lib != 0, -EINVAL, "rmdir failed\n");
#endif
					}
				}
				goto wait_for_mcos;
			}
		}
	} while (1);

out:
	if (devfd >= 0) {
		close(devfd);
	}
	if (evfd_kmsg >= 0) {
		close(evfd_kmsg);
	}
	if (evfd_status >= 0) {
		close(evfd_status);
	}
	if (epfd >= 0) {
		close(epfd);
	}
	arg->ret = ret;
	closelog();
	return NULL;
}

#define MCKUDEV_MAX_NUM_OS_INSTANCES 1

static void show_usage(char** argv) {
	printf("%s [--help|-?] [-f <facility_name>] [-k <redirect_kmsg>] [-n <detect_hungup>]\n"
		   "--help            \tShow usage\n"
		   "-f <facility_name>\tUse <facility_name> when redirecting kmsg by using syslog()\n"
		   "-k <redirect_kmsg>\t1: Redirect kmsg\n"
		   "                  \t0: Otherwise\n"
		   "-i <monitor_interval>\t!=-1: Polling interval (in second) for detecting hungup\n"
		   "                  \t-1: Don't detect hungup\n",
		   strrchr(argv[0], '/') + 1);
}

int main(int argc, char** argv) {
	int ret = 0, ret_lib;
	int opt;
	int evfd_mcos = -1, epfd = -1;
	struct epoll_event event;
	struct epoll_event events[1];
	int i;
	struct udev *udev = NULL;
	struct udev_monitor *mon_mcos = NULL;
	pthread_attr_t attr;
	struct thr_args mon_args[MCKUDEV_MAX_NUM_OS_INSTANCES];
	struct thr_args kmsg_args[MCKUDEV_MAX_NUM_OS_INSTANCES];
	int facility = LOG_LOCAL6;
	int enable_kmsg = 1;
	int mon_interval = 600; /* sec */

	while ((opt = getopt_long(argc, argv, "f:k:i:", longopt, NULL)) != -1) {
		switch (opt) {
		case 'f':
			for (i = 0; i < 8; i++) {
				if (strcmp(optarg, facility_list[i].name) == 0) {
					facility = facility_list[i].code;
					goto found;
				}
			}
			CHKANDJUMP(1, 255, "Unknown facility\n");
		found:;
			break;
		case 'k':
			enable_kmsg = atoi(optarg);
			break;
		case 'i':
			mon_interval = atoi(optarg);
			break;
		case '?':
		default:
			show_usage(argv);
			break;
		}
	}

	dprintf("enable_kmsg=%d,mon_interval=%d\n", enable_kmsg, mon_interval);

#ifdef DEBUG
	daemon(1, 1);
#else
	daemon(1, 0);
#endif

	ret_lib = pthread_attr_init(&attr);
	CHKANDJUMP(ret_lib != 0, 255, "pthread_attr_init returned %s\n", strerror(ret_lib));
	ret_lib = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	CHKANDJUMP(ret_lib != 0, 255, "pthread_attr_setdetachstate returned %s\n", strerror(ret_lib));

	for (i = 0; i < MCKUDEV_MAX_NUM_OS_INSTANCES; i++) {
		if (mon_interval != -1) {
			mon_args[i].dev_index = 0;
			mon_args[i].os_index = i;
			mon_args[i].interval = mon_interval;
			mon_args[i].evfd_mcos_removed = eventfd(0, 0);
			CHKANDJUMP(mon_args[i].evfd_mcos_removed == -1, 255, "eventfd failed\n");
			
			ret_lib = pthread_cond_init(&mon_args[i].cond_mcos_added, NULL);
			CHKANDJUMP(ret_lib != 0, 255, "pthread_cond_init returned %s\n", strerror(ret_lib));
			
			ret_lib = pthread_mutex_init(&mon_args[i].lock, NULL);
			CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_init returned %s\n", strerror(ret_lib));
			mon_args[i].mcos_added = 0;
			
			ret_lib = pthread_create(&mon_args[i].thread, &attr, detect_hungup, &mon_args[i]);
			CHKANDJUMP(ret_lib != 0, 255, "pthread_create returned %s\n", strerror(ret_lib));
		}

		if (enable_kmsg) {
			kmsg_args[i].dev_index = 0;
			kmsg_args[i].os_index = i;
			kmsg_args[i].logid = strrchr(argv[0], '/') + 1;
			kmsg_args[i].facility = facility;
			kmsg_args[i].evfd_mcos_removed = eventfd(0, 0);
			CHKANDJUMP(kmsg_args[i].evfd_mcos_removed == -1, 255, "eventfd failed\n");
			
			ret_lib = pthread_cond_init(&kmsg_args[i].cond_mcos_added, NULL);
			CHKANDJUMP(ret_lib != 0, 255, "pthread_cond_init returned %s\n", strerror(ret_lib));

			ret_lib = pthread_mutex_init(&kmsg_args[i].lock, NULL);
			CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_init returned %s\n", strerror(ret_lib));
			kmsg_args[i].mcos_added = 0;
			
			ret_lib = pthread_create(&kmsg_args[i].thread, &attr, redirect_kmsg, &kmsg_args[i]);
			CHKANDJUMP(ret_lib != 0, 255, "pthread_create returned %s\n", strerror(ret_lib));
		}
	}


	udev = udev_new();
	CHKANDJUMP(udev == NULL, 255, "udev_new failed\n");

	/* Obtain evfd for add /dev/mcosX event */
	mon_mcos = udev_monitor_new_from_netlink(udev, "udev");
	CHKANDJUMP(mon_mcos == NULL, 255, "udev_monitor_new_from_netlink failed\n");

	ret_lib = udev_monitor_filter_add_match_subsystem_devtype(mon_mcos, "mcos", NULL);
	CHKANDJUMP(ret_lib < 0, 255, "udev_monitor_filter_add_match_subsystem_devtype returned %s\n", strerror(-ret_lib));

	ret_lib = udev_monitor_enable_receiving(mon_mcos);
	CHKANDJUMP(ret_lib < 0, 255, "udev_monitor_enable_receiving %s\n", strerror(-ret_lib));
	
	evfd_mcos = udev_monitor_get_fd(mon_mcos);
	CHKANDJUMP(evfd_mcos < 0, 255, "udev_monitor_get_fd returned %s\n", strerror(-evfd_mcos));

	/* Add evfds to epoll fd */
	epfd = epoll_create(1);
	CHKANDJUMP(epfd == -1, 255, "epoll_create failed\n");
	
	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = evfd_mcos;
	ret_lib = epoll_ctl(epfd, EPOLL_CTL_ADD, evfd_mcos, &event);
	CHKANDJUMP(ret_lib != 0, 255, "epoll_ctl failed\n");

	do {
		int nfd = epoll_wait(epfd, events, 1, -1);
		for (i = 0; i < nfd; i++) {
			if (events[i].data.fd == evfd_mcos) {
#define SZ_LINE 256
				char action[SZ_LINE];
				char node[SZ_LINE];
				struct udev_device *dev;
				int os_index;

				/* Don't reap_event(evfd), it's harmful. */
				dev = udev_monitor_receive_device(mon_mcos);
				CHKANDJUMP(dev == NULL, 255, "udev_monitor_receive_device failed\n");
				
				strncpy(node, udev_device_get_devnode(dev), SZ_LINE);
				node[SZ_LINE - 1] = 0;
				dprintf("Node: %s\n", node);

				os_index = atoi(node + 4);

				strncpy(action, udev_device_get_action(dev), SZ_LINE);
				action[SZ_LINE - 1] = 0;
				
				if (strcmp(action, "add") == 0) {
					dprintf("mcos add detected\n");
					if (mon_interval != -1) {
						ret_lib = pthread_mutex_lock(&mon_args[os_index].lock);
						CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_lock failed\n");
						mon_args[os_index].mcos_added = 1;
						ret_lib = pthread_cond_signal(&mon_args[os_index].cond_mcos_added);
						if (ret_lib != 0) {
							pthread_mutex_unlock(&mon_args[os_index].lock);
							eprintf("pthread_cond_signal failed\n");
							ret = 255;
							goto out;

						}
						ret_lib = pthread_mutex_unlock(&mon_args[os_index].lock);
						CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_unlock failed\n");
					}
					if (enable_kmsg) {
						ret_lib = pthread_mutex_lock(&kmsg_args[os_index].lock);
						CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_lock failed\n");
						kmsg_args[os_index].mcos_added = 1;
						ret_lib = pthread_cond_signal(&kmsg_args[os_index].cond_mcos_added);
						if (ret_lib != 0) {
							pthread_mutex_unlock(&mon_args[os_index].lock);
							eprintf("pthread_cond_signal failed\n");
							ret = 255;
							goto out;

						}
						ret_lib = pthread_mutex_unlock(&kmsg_args[os_index].lock);
						CHKANDJUMP(ret_lib != 0, 255, "pthread_mutex_unlock failed\n");
					}
				} else if (strcmp(action, "remove") == 0) {
					uint64_t counter = 1;
					dprintf("mcos remove detected\n");
					if (mon_interval != -1) {
						ret_lib = write(mon_args[os_index].evfd_mcos_removed, &counter, sizeof(counter));
						CHKANDJUMP(ret_lib != sizeof(counter), 255, "write failed\n");
					}
					if (enable_kmsg) {
						ret_lib = write(kmsg_args[os_index].evfd_mcos_removed, &counter, sizeof(counter));
						CHKANDJUMP(ret_lib != sizeof(counter), 255, "write failed\n");
					}
				}
			}
		}
	} while (1);
 out:
	udev_monitor_unref(mon_mcos);
	udev_unref(udev);
	for (i = 0; i < MCKUDEV_MAX_NUM_OS_INSTANCES; i++) {
		if (mon_interval != -1) {
			if (mon_args[i].evfd_mcos_removed != -1) {
				close(mon_args[i].evfd_mcos_removed);
			}
		}
		if (enable_kmsg) {
			if (kmsg_args[i].evfd_mcos_removed != -1) {
				close(kmsg_args[i].evfd_mcos_removed);
			}
		}
	}
	if (evfd_mcos != -1) {
		close(evfd_mcos);
	}
	if (epfd != -1) {
		close(epfd);
	}
	return ret;
}
