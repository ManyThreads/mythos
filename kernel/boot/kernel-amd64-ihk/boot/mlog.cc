/* -*- mode:C++; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */

#include "boot/mlog.hh"
#include "cpu/MLogSinkSerial.hh"
#include "boot/memory-layout.h"
#include <new>

extern void mythos::boot::memcpy_ringbuf(char const * buf, int len);

struct ihk_kmsg_buf {
	int lock; /* Be careful, it's inter-kernel lock */
	int tail;
	int len;
	int head;
	char padding[4096 - sizeof(int) * 4]; /* Alignmment needed for some systems */
	char str[IHK_KMSG_SIZE];
};

extern struct ihk_kmsg_buf *kmsg_buf;

#define IHK_KMSG_SIZE            (8192 << 5)
#define DEBUG_KMSG_MARGIN (kmsg_buf->head == kmsg_buf->tail ? kmsg_buf->len : (((unsigned int)kmsg_buf->head - (unsigned int)kmsg_buf->tail) % (unsigned int)kmsg_buf->len))

namespace mythos {
  namespace boot {

class MySink : public mlog::ISink{
	public:
    void write(char const* buf, size_t len){
		if (kmsg_buf == NULL) {
			return;
		}

		while(__sync_val_compare_and_swap(&kmsg_buf->lock, 0, 1) != 0){
		   asm volatile("pause" ::: "memory");
		}

		bool overflow = DEBUG_KMSG_MARGIN < len;

		memcpy_ringbuf(buf, len);
		memcpy_ringbuf("\n", 1);
		
		if (overflow) {
			kmsg_buf->head = (kmsg_buf->tail + 1) % kmsg_buf->len;
		}
		kmsg_buf->lock = 0;
	}

	void flush(){};
};

	alignas(MySink) char mlogsink[sizeof(MySink)];

    void initMLog()
    {
      // need constructor for vtable before global constructors were called
      auto s = new(mlogsink) MySink();
      mlog::sink = s; // now logging is working
      mlog::boot.setName("boot"); // because constructor is still not called
    }
  } // namespace boot
} // namespace mythos


namespace mlog {
  Logger<MLOG_BOOT> boot("boot");
} // namespace mlog
