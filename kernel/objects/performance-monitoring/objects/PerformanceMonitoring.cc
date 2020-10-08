#include "objects/PerformanceMonitoring.hh"

#include "async/SynchronousTask.hh"
#include "cpu/hwthread_pause.hh"

namespace mythos {

	PerformanceMonitoring::PerformanceMonitoring()
	{
		for(cpu::ThreadID threadID = 0; threadID < MYTHOS_MAX_THREADS; ++threadID) {
			collectTasklets[threadID].initCollectFunctor(this);
		}

		counterNumber.offcoreRspMsrs = MAX_NUMBER_OFFCORE_RSP_MSRS;

		mythos::x86::Regs regs = mythos::x86::cpuid(0x00);
		//mlogpm.info("Maximum Input Value for Basic CPUID Information:", DVAR(regs.eax));
		ASSERT(regs.eax>=0x0A);

		regs = mythos::x86::cpuid(0x0A); //get Architectural Performance Monitoring Information

		counterNumber.pmcs = mythos::bits(regs.eax,15,8);
		//mlogpm.info("number of pmcs supportet by hardware", DVAR(counterNumber.pmcs));
		counterNumber.fixedCtrs = mythos::bits(regs.edx,5,0);
		//mlogpm.info("number of fixed counters supportet by hardware", DVAR(counterNumber.fixedCtrs));

		if(counterNumber.pmcs > MAX_NUMBER_PMCS) {
			counterNumber.pmcs = MAX_NUMBER_PMCS;
			mlogpm.info("Hardware supports more performance counters than MAX_NUMBER_PMCS.");
		}
		if(counterNumber.fixedCtrs > MAX_NUMBER_FIXED_CTRS) {
			counterNumber.fixedCtrs = MAX_NUMBER_FIXED_CTRS;
			mlogpm.info("Hardware supports more fixed performance counters than MAX_NUMBER_FIXED_CTRS.");
		}

		//regs = mythos::x86::cpuid(0x01);
		//mlogpm.info("family ID:", DVAR(mythos::bits(regs.eax,11,8)));
		//mlogpm.info("model ID:", DVAR((mythos::bits(regs.eax,19,16)<<4)+mythos::bits(regs.eax,7,4)));

		//regs = mythos::x86::cpuid(0x80000007);
		//mlogpm.info("invariant tsc:", DVAR(mythos::bits(regs.edx,8,8)));

		//regs = mythos::x86::cpuid(0x15); //get Time Stamp Counter Information
		//mlogpm.info("nominal core crystal clock frequency in Hz:", DVAR(regs.ecx));
		//mlogpm.info("TSC/\"core crystal clock\" ratio numerator:", DVAR(regs.ebx));
		//mlogpm.info("TSC/\"core crystal clock\" ratio denominator:", DVAR(regs.eax));

		//uint64_t tscHz = static_cast<double>(regs.ecx) * (static_cast<double>(regs.ebx)/static_cast<double>(regs.eax));
		//mlogpm.info(DVAR(tscHz));
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
		//printx2APIC_ID();

		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, 0);
		for(uint32_t index = 0; index<counterNumber.fixedCtrs; ++index) {
			mythos::x86::setMSR(IA32_FIXED_CTRx_STARTADDRESS + index, 0);
		}
		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, configValues[configValueIndex].ia32_fixed_ctr_ctrl);

		for(uint32_t index = 0; index<counterNumber.offcoreRspMsrs; ++index) {
			mythos::x86::setMSR(MSR_OFFCORE_RSP_x_STARTADDRESS + index, configValues[configValueIndex].msr_offcore_rsp_[index]);
		}

		for(uint32_t index = 0; index<counterNumber.pmcs; ++index) {
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
		for(uint32_t index = 0; index<counterNumber.fixedCtrs; ++index) {
			collectedValues[threadID].fixedCtrValues[index] = mythos::x86::getMSR(IA32_FIXED_CTRx_STARTADDRESS + index);
		}

		for(uint32_t index = 0; index<counterNumber.pmcs; ++index) {
			collectedValues[threadID].pmcValues[index] = mythos::x86::getMSR(IA32_PMCx_STARTADDRESS + index);
		}

		//inform other threads that this threads local performance monitoring counter values have been saved
		collectRequests.outstanding[threadID].store(false, std::memory_order_release);
		//Todo: Make shure the compiler doesn't reorder the following instructions before the informing instruction
		//because the informing should happen as soon as possible.

		//reset performance monitoring counter values to zero and continue counting
		uint64_t fixedctrctrl = mythos::x86::getMSR(IA32_FIXED_CTR_CTRL_ADDRESS);
		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, fixedctrctrl & 0x1100110011001100);
		for(uint32_t index = 0; index<counterNumber.fixedCtrs; index+=1) {
			mythos::x86::setMSR(IA32_FIXED_CTRx_STARTADDRESS + index, 0);
		}
		mythos::x86::setMSR(IA32_FIXED_CTR_CTRL_ADDRESS, fixedctrctrl);

		for(uint32_t index = 0; index<counterNumber.pmcs; ++index) {
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
		for (cpu::ThreadID threadID=0; threadID<cpu::getNumThreads(); ++threadID) {
			mlogpm.info(DVAR(threadID),DVAR(getCollectionTimeStamp(threadID)));
			for(size_t index = 0; index<counterNumber.fixedCtrs; ++index) {
				mlogpm.info(DVAR(threadID),DVAR(index),DVAR(getFixedCtr(threadID,index)));
			}
			for(size_t index = 0; index<counterNumber.pmcs; ++index) {
				mlogpm.info(DVAR(threadID),DVAR(index),DVAR(getPMC(threadID,index)));
			}
		}
		mlogpm.info("printCollectedValues end");
	}

	optional<void const*> PerformanceMonitoring::vcast(TypeId id) const
	{
		mlogpm.info("vcast", DVAR(this), DVAR(id.debug()));
		if (id == typeId<PerformanceMonitoring>()) return /*static_cast<ExampleObj const*>*/(this);
		THROW(Error::TYPE_MISMATCH);
	}

	optional<void> PerformanceMonitoring::deleteCap(CapEntry&, Cap self, IDeleter& del)
	{
		if (self.isOriginal()) {
			del.deleteObject(del_handle);
		}
		RETURN(Error::SUCCESS);
	}

	void PerformanceMonitoring::invoke(Tasklet* t, Cap self, IInvocation* msg){
		//mlogpm.info("invoke");
		monitor.request(t, [=](Tasklet* t){
			Error err = Error::NOT_IMPLEMENTED;
			switch (msg->getProtocol()) {
				case protocol::PerformanceMonitoring::proto:
				err = protocol::PerformanceMonitoring::dispatchRequest(this, msg->getMethod(), t, self, msg);
				break;
			}
			if (err != Error::INHIBIT) {
				msg->replyResponse(err);
				monitor.requestDone();
			}
        	});
	}

	Error PerformanceMonitoring::invoke_initializeCounters(Tasklet*, Cap, IInvocation*){
		mlogpm.info("invoke_initializeCounters");
		//mlogpm.info(DVAR(readTSC()));

		setFixedCtrCtrl(0, true, true, false, false);
		setFixedCtrCtrl(1, true, false, false, false);
		setFixedCtrCtrl(2, true, true, false, false);

		IA32_PERFEVTSELxBitfield evtsel(0);
		evtsel.userMode = true;
		evtsel.operatingSystemMode = true;
		evtsel.enableCounters = true;
		//evtsel.anyThread = true;

		//UnHalted Reference Cycles
		//evtsel.eventSelect = 0x3C;
		//evtsel.unitMask = 0x01;
		//setPerfevtsel(0, evtsel);

		//MEM_LOAD_UOPS_LLC_HIT_RETIRED.XSNP_HITM on sandy bridge and ivy bridge
		evtsel.eventSelect = 0xD2;
		evtsel.unitMask = 0x04;
		setPerfevtsel(2, evtsel);

		//OFFCORE_RESPONSE_0 on Sandy Bridge and Ivy Bridge
		setOffcoreRsp(0, 0x10002C0003); //DEMAND_DATA_RD & DEMAND_RFO with supplier LLC_HITM & LLC_HITE & LLC_HITF and HITM
		evtsel.eventSelect = 0xB7;
		evtsel.unitMask = 0x01;
		setPerfevtsel(0, evtsel);

		//LLC misses
		evtsel.eventSelect = 0x2E;
		evtsel.unitMask = 0x41;
		setPerfevtsel(3, evtsel);

		//MEM_LOAD_UOPS_LLC_MISS_RETIRED.LOCAL_DRAM on Ivy Bridge-E
		evtsel.eventSelect = 0xD3;
		evtsel.unitMask = 0x03;
		setPerfevtsel(1, evtsel);

		//off-core on Sandy Bridge / Ivy Bridge
		evtsel.eventSelect = 0xBB;
		evtsel.unitMask = 0x01;
		setPerfevtsel(2, evtsel);
		//setOffcoreRsp(1, 0x10001); //DEMAND_DATA_RD with ANY response type
		//setOffcoreRsp(1, 0x80400001); //DEMAND_DATA_RD with supplier LOCAL and SNP_NONE
		//setOffcoreRsp(1, 0x100400001); //DEMAND_DATA_RD with supplier LOCAL and SNP_NOT_NEEDED
		//setOffcoreRsp(1, 0x10002C8FFF); //all request types with supplier LLC_HITM & LLC_HITE & LLC_HITF and HITM
		//setOffcoreRsp(1, 0x10002C0033); //DEMAND_DATA_RD & DEMAND_RFO & PF_DATA_RD & PF_RFO with supplier LLC_HITM & LLC_HITE & LLC_HITF and HITM
		setOffcoreRsp(1, 0x10002C0030); //PF_DATA_RD & PF_RFO with supplier LLC_HITM & LLC_HITE & LLC_HITF and HITM

		for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); ++threadID) {
			initializeCounters(threadID);
		}

		//mlogpm.info(DVAR(readTSC()));
		return Error::SUCCESS;
	}

	Error PerformanceMonitoring::invoke_collectValues(Tasklet*, Cap, IInvocation*){
		mlogpm.info("invoke_collectValues");
		for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); ++threadID) {
			collect(threadID);
		}
		for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); ++threadID) {
			waitForCollect(threadID);
		}
		return Error::SUCCESS;
	}

	Error PerformanceMonitoring::invoke_printValues(Tasklet*, Cap, IInvocation*){
		mlogpm.info("invoke_printValues");
		printCollectedValues();
		return Error::SUCCESS;
	}

	Error PerformanceMonitoring::invoke_measureCollectLatency(Tasklet*, Cap, IInvocation*){
		mlogpm.info("invoke_measureCollectLatency");
		mlogpm.info("collect requester:", DVAR(cpu::getThreadID()));

		auto  measure = [this](cpu::ThreadID first, cpu::ThreadID last){
			ASSERT(first<=last);
			ASSERT(last<cpu::getNumThreads());
			mlogpm.info("collect range:", DVAR(first), DVAR(last));
			constexpr size_t RUNS = 100;
			constexpr double TSC_TICS_PER_NSEC = 2.6;
			constexpr size_t LATENCY_STR_SIZE = RUNS * 20;

			mythos::FixedStreamBuf<LATENCY_STR_SIZE> latencyStr;
			mythos::ostream_base<mythos::FixedStreamBuf<LATENCY_STR_SIZE>> latencyOStream(&latencyStr);

			uint64_t min = 0xFFFFFFFFFFFFFFFF;
			uint64_t max = 0;
			uint64_t average = 0;

			for(size_t run = 0; run < RUNS; ++run) {
				uint64_t timestamp1;
				uint64_t timestamp2;
				uint64_t latency;

				timestamp1=readTSC();
				for(cpu::ThreadID threadID = first; threadID <= last; ++threadID) {
					collect(threadID);
				}
				//timestamp2=readTSC();
				//timestamp1=readTSC();
				for(cpu::ThreadID threadID = first; threadID <= last; ++threadID) {
					waitForCollect(threadID);
				}
				timestamp2=readTSC();

				latency=timestamp2-timestamp1;
				mlogpm.info(DVAR(timestamp1),DVAR(timestamp2),DVAR(latency));

				if(min>latency)
					min = latency;

				if(max<latency)
					max = latency;

				average += latency;
				latencyOStream << static_cast<uint64_t>(static_cast<double>(latency)/TSC_TICS_PER_NSEC) << ";";
			}

			mlogpm.info("latencies in nanoseconds:");
			mlog::sink->write(latencyStr.c_str(),latencyStr.size());

			average /= RUNS;
			mlogpm.info(DVAR(min),DVAR(max),DVAR(average));
		};

		for(cpu::ThreadID id=0; id < cpu::getNumThreads(); ++id) {
			measure(id,id);
		}
		for(cpu::ThreadID id=1; id < cpu::getNumThreads(); ++id) {
			measure(1,id);
		}
		measure(0,cpu::getNumThreads()-1);

		return Error::SUCCESS;
	}

	Error PerformanceMonitoring::invoke_measureSpeedup(Tasklet*, Cap, IInvocation*){
		//mlogpm.info("invoke_measureSpeedup");

		//size_t threadNumber = cpu::getNumThreads();
		size_t threadNumber = 6;
		uint64_t xHitmLatency = 60;

		uint64_t intervalStart = getCollectionTimeStampLocal();
		uint64_t unhaltedSum = 0;
		uint64_t xsnpHitmSum = 0;

		for(cpu::ThreadID threadID = 0; threadID < threadNumber; ++threadID) {
			collect(threadID);
		}
		for(cpu::ThreadID threadID = 0; threadID < threadNumber; ++threadID) {
			waitForCollect(threadID);
			uint64_t unhalted = getFixedCtr(threadID,2);
			uint64_t xsnpHitm = getPMC(threadID,0);
			unhaltedSum+=unhalted;
			xsnpHitmSum+=xsnpHitm;
			mlogpm.info(DVAR(threadID),DVAR(unhalted),DVAR(xsnpHitm));
		}

		if(intervalStart==0) {
			mlogpm.warn("Unknown start of measurement interval. -> No reliable speedup calculation possible.");
		}

		uint64_t intervalEnd = getCollectionTimeStampLocal();
		ASSERT(intervalEnd > intervalStart);
		uint64_t intervalLength = intervalEnd - intervalStart;

		uint64_t haltDurationSum = intervalLength*threadNumber;
		if(haltDurationSum > unhaltedSum)
			haltDurationSum -= unhaltedSum;
		uint64_t xHitmLoadDurationSum =  xsnpHitmSum*xHitmLatency;

		uint64_t executionDelaySum = haltDurationSum + xHitmLoadDurationSum;
		uint64_t calculatedSeqRuntime = 0;
		if(executionDelaySum < intervalLength*threadNumber)
			calculatedSeqRuntime = intervalLength*threadNumber - executionDelaySum;

		uint64_t speedup = (calculatedSeqRuntime * 10000) / intervalLength;
		uint64_t efficiency = speedup / threadNumber;
		speedup = (speedup+5)/10;
		efficiency = (efficiency+5)/10;

		mlogpm.info("time stamp:",DVAR(intervalStart),"; time stamp:",DVAR(intervalEnd),";",DVAR(intervalLength),"tsc cycles;",DVAR(threadNumber));
		mlogpm.info(DVAR(haltDurationSum),"tsc cycles;",DVAR(xHitmLoadDurationSum),"tsc cycles (rough estimation);",DVAR(calculatedSeqRuntime),"tsc cycles");
		mlogpm.info(DVAR(speedup),"*1^(-3);",DVAR(efficiency),"*1^(-3)");

		//uint64_t seqRuntime = intervalLength; //seqRuntime for threadNumber = 1
		//uint64_t seqRuntime = 2853632468; //seqRuntime for primeFactors_omp
		uint64_t seqRuntime = 261453731; //seqRuntime for multipleProdurcers_omp
		uint64_t speedupClassicCalc = ((seqRuntime*10000/intervalLength)+5)/10;
		uint64_t speedupAbsDiff = speedupClassicCalc - speedup;
		if(speedupClassicCalc < speedup)
			speedupAbsDiff = speedup - speedupClassicCalc;
		if(speedupMaxAbsDiff<speedupAbsDiff)
			speedupMaxAbsDiff = speedupAbsDiff;
		mlogpm.info(DVAR(speedupClassicCalc),"*^(-3)");

		//measurement logs for primeFactors_omp:
		//mlogpm.info("log1:intervalLength log2:speedup");//for threadNumber = 1 to get seqRuntime
		//measurementLog1.log(intervalLength);
		//mlogpm.info("log1:speedupClassicCalc with",DVAR(seqRuntime)," log2:speedup");
		//measurementLog1.log(speedupClassicCalc);
		//measurementLog2.log(speedup);

		//measurementlogs for multipleProducers_omp:
		for(cpu::ThreadID threadID = 0; threadID < threadNumber; ++threadID) {
			uint64_t xsnpHitmPrefetchReads = getPMC(threadID,2);
			mlogpm.info(DVAR(threadID),DVAR(xsnpHitmPrefetchReads));
		}
		ASSERT(intervalLength*threadNumber>=seqRuntime);
		uint64_t executionDelaySumClassicCalc = intervalLength*threadNumber-seqRuntime;
		//mlogpm.info("log1:intervalLength log2:executionDelaySum");//for threadNumber = 1 to get seqRuntime
		//measurementLog1.log(intervalLength);
		mlogpm.info("log1:executionDelaySumClassicCalc log2:executionDelaySum");
		measurementLog1.log(executionDelaySumClassicCalc);
		measurementLog2.log(executionDelaySum);

		++measNum;
		constexpr size_t MEASUREMENT_NUMBER = 25;
		if(measNum>=MEASUREMENT_NUMBER){
			constexpr size_t MEASUREMENT_STR_SIZE = MEASUREMENT_NUMBER * 2 * 20;

			mythos::FixedStreamBuf<MEASUREMENT_STR_SIZE> measurementStr;
			mythos::ostream_base<mythos::FixedStreamBuf<MEASUREMENT_STR_SIZE>> measurementOStream(&measurementStr);

			uint64_t log1sum=0;
			uint64_t log2sum=0;

			measurementOStream << "log1:";
			for(size_t i=0; i<MEASUREMENT_NUMBER; ++i){
				measurementOStream << measurementLog1.getValue(i) << ";";
				log1sum += measurementLog1.getValue(i);
			}
			measurementOStream << "log2:";
			for(size_t i=0; i<MEASUREMENT_NUMBER; ++i){
				measurementOStream << measurementLog2.getValue(i) << ";";
				log2sum += measurementLog2.getValue(i);
			}
			mlogpm.info(DVAR(log1sum),DVAR(log2sum),"logged values:");
			mlog::sink->write(measurementStr.c_str(),measurementStr.size());
			mlogpm.info(DVAR(speedupMaxAbsDiff),"*10^(-3)");
		}

		return Error::SUCCESS;
	}

} // mythos
