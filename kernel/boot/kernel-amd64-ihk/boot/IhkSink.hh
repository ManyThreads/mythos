#pragma once

#include "util/ISink.hh"
#include "util/TidexMutex.hh"
#include "cpu/hwthreadid.hh"
#include "boot/ihk-entry.hh"

namespace mythos {

class IhkSink : public mlog::ISink {
	public:
    void write(char const* buf, size_t len){
    	using namespace mythos::boot;
    	//auto id = cpu::getThreadID();
    	mutex << [this,buf,len]() {
      		
      		// TODO: cleanup, output thread id

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
			kmsg_buf->lock = 0; // TODO: this cant be safe ...

    	};
	}

	void flush(){};
  protected:
    TidexMutex<KernelMutexContext> mutex;
};

} // namespace mythos
