#pragma once

#include "async/NestedMonitorDelegating.hh"
#include "objects/IKernelObject.hh"
#include "objects/IFactory.hh"
#include "util/assert.hh"
#include "objects/DebugMessage.hh"
#include "objects/ops.hh"
#include "mythos/protocol/PerfMon.hh"
#include "cpu/hwthreadid.hh"
#include "util/optional.hh"
#include "util/bitfield.hh"
#include "cpu/ctrlregs.hh"
#include "cpu/LAPIC.hh"
#include "async/Place.hh"
#include "async/Tasklet.hh"
#include <atomic>

namespace mythos{

static mlog::Logger<mlog::FilterAny> mlogpm("PerformanceMonitoring");

class PerformanceMonitoring
	: public IKernelObject
{
	public: 

		BITFIELD_DEF(uint64_t, IA32_PERFEVTSELxBitfield)
		UIntField<value_t,base_t,0,8> eventSelect;
		UIntField<value_t,base_t,8,8> unitMask;
		BoolField<value_t,base_t,16> userMode;
		BoolField<value_t,base_t,17> operatingSystemMode;
		BoolField<value_t,base_t,18> edgeDetect;
		BoolField<value_t,base_t,19> pinControl;
		BoolField<value_t,base_t,20> apicInteruptEnable;
		BoolField<value_t,base_t,21> anyThread;
		BoolField<value_t,base_t,22> enableCounters;
		BoolField<value_t,base_t,23> invert;
		UIntField<value_t,base_t,24,8> counterMask;
		BITFIELD_END

		class CollectBroadcastFunctor
		{
			public:
				CollectBroadcastFunctor(PerformanceMonitoring& performanceMonitoring)
				: pm(performanceMonitoring)
				{
				}
				void operator()() const {
					for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); threadID+=1) {
						pm.collect(threadID);
					}
					for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); threadID+=1) {
						pm.waitForCollect(threadID);
					}
				}
			private:
				PerformanceMonitoring& pm;
		};

		PerformanceMonitoring();

		void init(){
			if(isInitialized) return;
			mlogpm.info("PerformanceMonitoring isInitialized");
		}


		void setPerfevtsel(size_t index, const IA32_PERFEVTSELxBitfield& perfevtsel) {
			ASSERT(index < counterNumber.pmcs);
			configValues[cpu::getThreadID()].ia32_perfevtsel[index] = perfevtsel.value;
		}
		void setOffcoreRsp(size_t index, uint64_t offcoreRsp) {
			ASSERT(index < counterNumber.offcoreRspMsrs);
			configValues[cpu::getThreadID()].msr_offcore_rsp_[index];
		}
		void setFixedCtrCtrl(size_t index, bool enableOS, bool enableUser, bool anyThread, bool enablePMI);


		void initalizeCountersLocal() {
			initalizeCountersLocal(cpu::getThreadID());
		}
		void initializeCounters(cpu::ThreadID threadID);


		//collect counter values: save current counter values and reset counters to zero.
		void collectLocal();
		void collect(cpu::ThreadID threadID); //issue collect request to a thread.
		void waitForCollect(cpu::ThreadID threadID); //wait for outstanding collect request.


		void printCollectedValues() const;


		uint64_t getCollectionTimeStamp(cpu::ThreadID threadID) const {
			ASSERT(threadID < cpu::getNumThreads());
			return collectedValues[threadID].timeStamp;
		}
		uint64_t getCollectionTimeStampLocal() const {
			return getCollectionTimeStamp(cpu::getThreadID());
		}

		uint64_t getFixedCtr(cpu::ThreadID threadID, size_t index) const {
			ASSERT(threadID < cpu::getNumThreads());
			ASSERT(index < counterNumber.fixedCtrs);
			return collectedValues[threadID].fixedCtrValues[index];
		}
		uint64_t getFixedCtrLocal(size_t index) const {
			return getFixedCtr(cpu::getThreadID(), index);
		}

		uint64_t getPMC(cpu::ThreadID threadID, size_t index) const {
			ASSERT(threadID < cpu::getNumThreads());
			ASSERT(index < counterNumber.pmcs);
			return collectedValues[threadID].pmcValues[index];
		}
		uint64_t getPMCLocal(size_t index) const {
			return getPMC(cpu::getThreadID(), index);
		}


		CollectBroadcastFunctor getCollectBroadcastFunctor() {
			return CollectBroadcastFunctor(*this);
		}

	public: // IKernelInterface
		optional<void const*> vcast(TypeId id) const override;
		optional<void> deleteCap(CapEntry& entry, Cap self, IDeleter& del) override { RETURN(Error::SUCCESS); };
		void deleteObject(Tasklet* t, IResult<void>* r) override {};
		void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
		Error invoke_initializeCounters(Tasklet*, Cap, IInvocation* msg);
		Error invoke_collectValues(Tasklet*, Cap, IInvocation* msg);
		Error invoke_measureCollectCallDuration(Tasklet*, Cap, IInvocation* msg);
		Error invoke_measureCollectLatency(Tasklet*, Cap, IInvocation* msg);

	//CoreMap<> cmap;
	protected:
		async::NestedMonitorDelegating monitor;
		IDeleter::handle_t del_handle = {this};
	private:

		bool isInitialized = false;

		enum MSR_Adress {
			IA32_PMCx_STARTADDRESS = 0x0C1,
			IA32_PERFEVTSELx_STARTADDRESS = 0x186,
			MSR_OFFCORE_RSP_x_STARTADDRESS = 0x1A6,
			IA32_FIXED_CTRx_STARTADDRESS = 0x309,
			IA32_FIXED_CTR_CTRL_ADDRESS =0x38D
		};

		static constexpr size_t CLSIZE = 64;

		static constexpr size_t MAX_NUMBER_FIXED_CTRS = 3;
		static constexpr size_t MAX_NUMBER_OFFCORE_RSP_MSRS = 2;
		static constexpr size_t MAX_NUMBER_PMCS = 4;

		struct alignas(CLSIZE) ConfigValues
		{
			uint64_t ia32_fixed_ctr_ctrl;
			uint64_t msr_offcore_rsp_[MAX_NUMBER_OFFCORE_RSP_MSRS];
			uint64_t ia32_perfevtsel[MAX_NUMBER_PMCS];

			char padding[CLSIZE - (sizeof(ia32_fixed_ctr_ctrl)+sizeof(msr_offcore_rsp_)+sizeof(ia32_perfevtsel))%CLSIZE];
		};

		struct alignas(CLSIZE) PMCValues
		{
			uint64_t timeStamp;
			uint64_t fixedCtrValues[MAX_NUMBER_FIXED_CTRS];
			uint64_t pmcValues[MAX_NUMBER_PMCS];

			char padding[CLSIZE - (sizeof(timeStamp)+sizeof(fixedCtrValues)+sizeof(pmcValues))%CLSIZE];
		};

		struct alignas(CLSIZE) CounterNumber
		{
			size_t fixedCtrs;
			size_t offcoreRspMsrs;
			size_t pmcs;

			char padding[CLSIZE - (sizeof(fixedCtrs)+sizeof(offcoreRspMsrs)+sizeof(pmcs))%CLSIZE];
		};

		struct alignas(CLSIZE) CollectRequests
		{
			std::atomic_bool outstanding[MYTHOS_MAX_THREADS];

			char padding[CLSIZE - sizeof(outstanding)%CLSIZE];
		};

		class CollectFunctor
		{
			public:
				CollectFunctor() : pm(nullptr) {}
				void init(PerformanceMonitoring* performanceMonitoring) {
					pm = performanceMonitoring;
				}
				void operator()(async::TaskletBase* tb) const {
					ASSERT(tb->isUnused());
					tb->setInit();
					ASSERT(pm != nullptr);
					pm->collectLocal();
				}
			private:
				PerformanceMonitoring* pm;
		};

		class alignas(CLSIZE) CollectTasklet
			: public async::TaskletFunctor<CollectFunctor>
		{
			public:
				CollectTasklet() : async::TaskletFunctor<CollectFunctor>(CollectFunctor()) {
					static_assert(sizeof(CollectTasklet)%CLSIZE == 0, "padding is wrong");
				}
				void initCollectFunctor(PerformanceMonitoring* pm) {
					fun.init(pm);
				}
		};

		void initalizeCountersLocal(size_t configValueIndex);

		uint64_t readTSC() { //read time stamp counter
			uint32_t h,l;
			asm volatile ("rdtsc" : "=a"(l), "=d"(h));
			return (uint64_t(h) << 32) | l;
		}

		void printx2APIC_ID() {
			mythos::x86::Regs regs = mythos::x86::cpuid(0x0B);
			mlogpm.info("x2APIC ID:", DVAR(regs.edx));
			mlogpm.info("bits to shift right on x2APIC ID to get next topology level ID:", DVAR(mythos::bits(regs.eax,4,0)));
		}

		CounterNumber counterNumber;
		ConfigValues configValues[MYTHOS_MAX_THREADS];
		CollectTasklet collectTasklets[MYTHOS_MAX_THREADS];
		PMCValues collectedValues[MYTHOS_MAX_THREADS];
		CollectRequests collectRequests;
};


optional<void const*> PerformanceMonitoring::vcast(TypeId id) const
{
	mlogpm.info("vcast", DVAR(this), DVAR(id.debug()));
	if (id == typeId<PerformanceMonitoring>()) return /*static_cast<ExampleObj const*>*/(this);
	THROW(Error::TYPE_MISMATCH);
}

void PerformanceMonitoring::invoke(Tasklet* t, Cap self, IInvocation* msg){
	mlogpm.info("invoke");
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
      } );
}

Error PerformanceMonitoring::invoke_initializeCounters(Tasklet*, Cap, IInvocation* msg){
	auto data = msg->getMessage()->cast<protocol::PerformanceMonitoring::InitializeCounters>();

	msg->getMessage()->write<protocol::PerformanceMonitoring::InitializeCountersResponse>();

	auto resp = msg->getMessage()->cast<protocol::PerformanceMonitoring::InitializeCountersResponse>();

	mlogpm.info("invoke_initializeCounters");
	mlogpm.info(DVAR(readTSC()));

	setFixedCtrCtrl(0, true, true, false, false);
	setFixedCtrCtrl(1, true, true, false, false);
	setFixedCtrCtrl(2, true, true, false, false);

	IA32_PERFEVTSELxBitfield evtsel(0);
	evtsel.userMode = true;
	evtsel.operatingSystemMode = true;
	evtsel.enableCounters = true;

	//UnHalted Reference Cycles
	evtsel.eventSelect = 0x3C;
	evtsel.unitMask = 0x01;
	setPerfevtsel(0, evtsel);

	//MEM_LOAD_UOPS_LLC_HIT_RETIRED.XSNP_HITM on sandy bridge
	//evtsel.eventSelect = 0xD2;
	//evtsel.unitMask = 0x04;
	//setPerfevtsel(1, evtsel);

	//LLC misses
	evtsel.eventSelect = 0x2E;
	evtsel.unitMask = 0x41;
	setPerfevtsel(2, evtsel);

	for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); threadID+=1) {
		initializeCounters(threadID);
	}

	mlogpm.info(DVAR(readTSC()));
	return Error::SUCCESS;
}

Error PerformanceMonitoring::invoke_collectValues(Tasklet*, Cap, IInvocation* msg){
	auto data = msg->getMessage()->cast<protocol::PerformanceMonitoring::CollectValues>();

	msg->getMessage()->write<protocol::PerformanceMonitoring::CollectValuesResponse>();

	auto resp = msg->getMessage()->cast<protocol::PerformanceMonitoring::CollectValuesResponse>();

	mlogpm.info("invoke_collectValues");
	for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); threadID+=1) {
		collect(threadID);
	}
	for(cpu::ThreadID threadID = 0; threadID < cpu::getNumThreads(); threadID+=1) {
		waitForCollect(threadID);
	}

	printCollectedValues();
	return Error::SUCCESS;
}

Error PerformanceMonitoring::invoke_measureCollectCallDuration(Tasklet*, Cap, IInvocation* msg){
	auto data = msg->getMessage()->cast<protocol::PerformanceMonitoring::MeasureCollectCallDuration>();

	msg->getMessage()->write<protocol::PerformanceMonitoring::MeasureCollectCallDuration>();

	auto resp = msg->getMessage()->cast<protocol::PerformanceMonitoring::MeasureCollectCallDuration>();

	mlogpm.info("invoke_measureCollectCallDuration");
	constexpr cpu::ThreadID first = 1;
	constexpr cpu::ThreadID behindLast = 2;
	ASSERT(first<behindLast);
	ASSERT(behindLast<=cpu::getNumThreads());
	mlogpm.info("collect requester:", DVAR(cpu::getThreadID()));
	mlogpm.info("collect range:", DVAR(first), DVAR(behindLast));

	uint64_t timestamp1;
	uint64_t timestamp2;
	uint64_t latency;

	timestamp1=readTSC();
	for(cpu::ThreadID threadID = first; threadID < behindLast; threadID+=1) {
		collect(threadID);
	}
	timestamp2=readTSC();

	latency=timestamp2-timestamp1;
	mlogpm.info(DVAR(timestamp1),DVAR(timestamp2),DVAR(latency));

	return Error::SUCCESS;
}

Error PerformanceMonitoring::invoke_measureCollectLatency(Tasklet*, Cap, IInvocation* msg){
	auto data = msg->getMessage()->cast<protocol::PerformanceMonitoring::MeasureCollectLatency>();

	msg->getMessage()->write<protocol::PerformanceMonitoring::MeasureCollectLatencyResponse>();

	auto resp = msg->getMessage()->cast<protocol::PerformanceMonitoring::MeasureCollectLatencyResponse>();

	mlogpm.info("invoke_measureCollectLatency");
	constexpr cpu::ThreadID first = 1;
	constexpr cpu::ThreadID behindLast = 2;
	ASSERT(first<behindLast);
	ASSERT(behindLast<=cpu::getNumThreads());
	mlogpm.info("collect requester:", DVAR(cpu::getThreadID()));
	mlogpm.info("collect range:", DVAR(first), DVAR(behindLast));

	uint64_t timestamp1;
	uint64_t timestamp2;
	uint64_t latency;

	timestamp1=readTSC();
	for(cpu::ThreadID threadID = first; threadID < behindLast; threadID+=1) {
		collect(threadID);
	}
	for(cpu::ThreadID threadID = first; threadID < behindLast; threadID+=1) {
		waitForCollect(threadID);
	}
	timestamp2=readTSC();

	latency=timestamp2-timestamp1;
	mlogpm.info(DVAR(timestamp1),DVAR(timestamp2),DVAR(latency));

	//printCollectedValues();
	return Error::SUCCESS;
}

} //mythos
