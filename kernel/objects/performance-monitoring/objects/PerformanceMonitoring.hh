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

namespace mythos{

static mlog::Logger<mlog::FilterAny> mlogpm("PerformanceMonitoring");

class PerformanceMonitoring
	: public IKernelObject
{
	public: 
		PerformanceMonitoring()
		{
		}

		void init(){
			if(isInitialized) return;
			mlogpm.info("PerformanceMonitoring isInitialized");
		}

	public: // IKernelInterface
		optional<void const*> vcast(TypeId id) const override;
		optional<void> deleteCap(CapEntry& entry, Cap self, IDeleter& del) override { RETURN(Error::SUCCESS); };
		void deleteObject(Tasklet* t, IResult<void>* r) override {};
		void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
		Error invoke_startMonitoring(Tasklet*, Cap, IInvocation* msg);

	//CoreMap<> cmap;
	protected:
	  async::NestedMonitorDelegating monitor;
	  IDeleter::handle_t del_handle = {this};
	private:
	  bool isInitialized = false;
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

Error PerformanceMonitoring::invoke_startMonitoring(Tasklet*, Cap, IInvocation* msg){
	auto data = msg->getMessage()->cast<protocol::PerformanceMonitoring::StartMonitoring>();

	msg->getMessage()->write<protocol::PerformanceMonitoring::StartMonitoringResponse>();

	auto resp = msg->getMessage()->cast<protocol::PerformanceMonitoring::StartMonitoringResponse>();

	mlogpm.info("invoke_startMonitoring");

	return Error::SUCCESS;
}

} //mythos
