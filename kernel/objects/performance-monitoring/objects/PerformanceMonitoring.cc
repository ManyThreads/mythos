#include "objects/PerformanceMonitoring.hh"

#include "async/SynchronousTask.hh"
#include "cpu/hwthread_pause.hh"

namespace mythos {

	PerformanceMonitoring::PerformanceMonitoring()
	{
		for(cpu::ThreadID threadID = 0; threadID < MYTHOS_MAX_THREADS; threadID+=1) {
			collectTasklets[threadID].initCollectFunctor(this);
		}

		mythos::x86::Regs regs = mythos::x86::cpuid(0x00);
		mlogpm.info("Maximum Input Value for Basic CPUID Information:", DVAR(regs.eax));

		counterNumber.offcoreRspMsrs = MAX_NUMBER_OFFCORE_RSP_MSRS;

		regs = mythos::x86::cpuid(0x0A); //get Architectural Performance Monitoring Information

		counterNumber.pmcs = mythos::bits(regs.eax,15,8);
		mlogpm.info("number of pmcs supportet by hardware", DVAR(counterNumber.pmcs));
		counterNumber.fixedCtrs = mythos::bits(regs.edx,5,0);
		mlogpm.info("number of fixed counters supportet by hardware", DVAR(counterNumber.fixedCtrs));

		if(counterNumber.pmcs > MAX_NUMBER_PMCS) {
			counterNumber.pmcs = MAX_NUMBER_PMCS;
			mlogpm.info("MAX_NUMBER_PMCS", DVAR(counterNumber.pmcs));
		}
		if(counterNumber.fixedCtrs > MAX_NUMBER_FIXED_CTRS) {
			counterNumber.fixedCtrs = MAX_NUMBER_FIXED_CTRS;
			mlogpm.info("MAX_NUMBER_FIXED_CTRS", DVAR(counterNumber.fixedCtrs));
		}

		regs = mythos::x86::cpuid(0x80000007);
		mlogpm.info("invariant tsc:", DVAR(mythos::bits(regs.edx,8,8)));

		//regs = mythos::x86::cpuid(0x15); //get Time Stamp Counter Information
		//mlogpm.info("nominal core crystal clock frequency in Hz:", DVAR(regs.ecx));
		//mlogpm.info("TSC/\"core crystal clock\" ratio numerator:", DVAR(regs.ebx));
		//mlogpm.info("TSC/\"core crystal clock\" ratio denominator:", DVAR(regs.eax));

		//uint64_t tscHz = static_cast<double>(regs.ecx) * (static_cast<double>(regs.ebx)/static_cast<double>(regs.eax));
		//mlogpm.info(DVAR(tscHz));

		/*for(size_t i = 0; i < 20; i+=1) {
			mlogpm.info(DVAR(readTSC()/2000000000));
			hwthread_pause(100000000);
		}*/

		/*IA32_PERFEVTSELxBitfield evtsel(0);
		evtsel.eventSelect = 0xB7;
		evtsel.unitMask = 0x01;
		configValues.ia32_perfevtsel[0] = evtsel.value;
		evtsel.eventSelect = 0xBB;
		configValues.ia32_perfevtsel[1] = evtsel.value;*/
	}

	void PerformanceMonitoring::setFixedCtrCtrl(size_t index, bool enableOS, bool enableUser, bool anyThread, bool enablePMI) {
		ASSERT(index < counterNumber.fixedCtrs);
		uint64_t clearMask(~(15ull << (index * 4)));
		uint64_t setMask(static_cast<uint64_t>(enablePMI)&1);
		setMask = (setMask << 1) + (static_cast<uint64_t>(anyThread)&1);
		setMask = (setMask << 1) + (static_cast<uint64_t>(enableUser)&1);
		setMask = (setMask << 1) + (static_cast<uint64_t>(enableOS)&1);
		setMask = (setMask << (index * 4));
		configValues[cpu::getThreadID()].ia32_fixed_ctr_ctrl = (configValues[cpu::getThreadID()].ia32_fixed_ctr_ctrl & clearMask) | setMask;
	}

	void PerformanceMonitoring::initalizeCountersLocal(size_t configValueIndex) {
		//mlogpm.info("initalizeCountersLocal", DVAR(cpu::getThreadID()), DVAR(configValueIndex));

		printx2APIC_ID();

		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, 0);
		for(size_t index = 0; index<counterNumber.fixedCtrs; index+=1) {
			mythos::x86::setMSR(IA32_FIXED_CTRx_STARTADDRESS + index, 0);
		}
		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, configValues[configValueIndex].ia32_fixed_ctr_ctrl);

		/*for(size_t index = 0; index<counterNumber.offcoreRspMsrs; index+=1) {
			//mythos::x86::setMSR(MSR_OFFCORE_RSP_x_STARTADDRESS + index, 0x010001);
			mythos::x86::setMSR(MSR_OFFCORE_RSP_x_STARTADDRESS + index, configValues[configValueIndex].msr_offcore_rsp_[index]);
		}*/

		for(size_t index = 0; index<counterNumber.pmcs; index+=1) {
			IA32_PERFEVTSELxBitfield perfevtselValue(configValues[configValueIndex].ia32_perfevtsel[index]);
			bool enabled = perfevtselValue.enableCounters;

			perfevtselValue.enableCounters = false;
			mythos::x86::setMSR(IA32_PERFEVTSELx_STARTADDRESS + index, perfevtselValue.value);
			mythos::x86::setMSR(IA32_PMCx_STARTADDRESS + index, 0);

			perfevtselValue.enableCounters = enabled;
			mythos::x86::setMSR(IA32_PERFEVTSELx_STARTADDRESS + index, perfevtselValue.value);
		}
	}

	void PerformanceMonitoring::initializeCounters(cpu::ThreadID threadID) {
		ASSERT(threadID < cpu::getNumThreads());
		if(threadID == cpu::getThreadID()) {
			initalizeCountersLocal();
		} else {
			size_t configValueIndex = static_cast<size_t>(cpu::getThreadID());
			synchronousAt(async::getPlace(threadID)) << [this, configValueIndex]() {
			    this->initalizeCountersLocal(configValueIndex);
			};
		}
	}

	void PerformanceMonitoring::collectLocal() {
		cpu::ThreadID threadID = cpu::getThreadID();
		//mlogpm.info("collectLocal", DVAR(threadID));

		//save current time stamp counter value
		collectedValues[threadID].timeStamp = readTSC();

		//save current performance monitoring counter values
		for(size_t index = 0; index<counterNumber.fixedCtrs; index+=1) {
			collectedValues[threadID].fixedCtrValues[index] = mythos::x86::getMSR(IA32_FIXED_CTRx_STARTADDRESS + index);
		}

		for(size_t index = 0; index<counterNumber.pmcs; index+=1) {
			collectedValues[threadID].pmcValues[index] = mythos::x86::getMSR(IA32_PMCx_STARTADDRESS + index);
		}

		//inform other threads that this threads local performance monitoring counter values have been saved
		collectRequests.outstanding[threadID].store(false, std::memory_order_release);
		//make shure the compiler doesn't reorder the following instructions before the informing instruction
		//because the informing should happen as soon as possible.
		std::atomic_signal_fence(std::memory_order_acquire);

		//reset performance monitoring counter values to zero and continue counting
		uint64_t fixedctrctrl = mythos::x86::getMSR(IA32_FIXED_CTR_CTRL_ADDRESS);
		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, fixedctrctrl & 0x1100110011001100);
		for(size_t index = 0; index<counterNumber.fixedCtrs; index+=1) {
			mythos::x86::setMSR(IA32_FIXED_CTRx_STARTADDRESS + index, 0);
		}
		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, fixedctrctrl);

		for(size_t index = 0; index<counterNumber.pmcs; index+=1) {
			IA32_PERFEVTSELxBitfield evtsel(mythos::x86::getMSR(IA32_PERFEVTSELx_STARTADDRESS + index));
			bool pmcEnabled = evtsel.enableCounters;

			evtsel.enableCounters = false;
			mythos::x86::setMSR(IA32_PERFEVTSELx_STARTADDRESS + index, evtsel.value);
			mythos::x86::setMSR(IA32_PMCx_STARTADDRESS + index, 0);

			evtsel.enableCounters = pmcEnabled;
			mythos::x86::setMSR(IA32_PERFEVTSELx_STARTADDRESS + index, evtsel.value);
		}
	}

	void PerformanceMonitoring::collect(cpu::ThreadID threadID) {
		ASSERT(threadID < cpu::getNumThreads());
		//mlogpm.info("collect", DVAR(threadID));
		if(threadID == cpu::getThreadID()) {
			collectLocal();
		} else {
			bool outstanding = collectRequests.outstanding[threadID].exchange(true, std::memory_order_relaxed);
			if(!outstanding) {
				async::getPlace(threadID)->pushSync(&collectTasklets[threadID]);
			}
		}
	}

	void PerformanceMonitoring::waitForCollect(cpu::ThreadID threadID) {
		ASSERT(threadID < cpu::getNumThreads());
		while(collectRequests.outstanding[threadID].load(std::memory_order_acquire) == true) {
			hwthread_pause();
			//The following line makes shure no deadlock occurs, if the other thread waits for the execution of a synchronous task by this thread.
			getLocalPlace().processSyncTasks();
		}
	}

	void PerformanceMonitoring::printCollectedValues() const {
		mlogpm.info("printCollectedValues start");
		for (cpu::ThreadID threadID=0; threadID<cpu::getNumThreads(); threadID+=1) {
			mlogpm.info(DVAR(threadID));
			mlogpm.info(DVAR(collectedValues[threadID].timeStamp));
			for(size_t index = 0; index<counterNumber.fixedCtrs; index+=1) {
				mlogpm.info("fixedCtr", DVAR(index));
				mlogpm.info(DVAR(collectedValues[threadID].fixedCtrValues[index]));
			}

			for(size_t index = 0; index<counterNumber.pmcs; index+=1) {
				mlogpm.info("pmc", DVAR(index));
				mlogpm.info(DVAR(collectedValues[threadID].pmcValues[index]));
			}
		}
		mlogpm.info("printCollectedValues end");
	}

} // mythos
