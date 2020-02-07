#pragma once

#include "mythos/protocol/common.hh"

namespace mythos{
   namespace protocol{

	struct PerformanceMonitoring{
		constexpr static uint8_t proto = mythos::protocol::PERFMON;

	enum Methods : uint8_t {
		STARTMONITORING
	};
	
	struct StartMonitoring : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + STARTMONITORING;
        StartMonitoring() 
			: InvocationBase(label,getLength(this)) 
		{
		}
      };

	struct StartMonitoringResponse : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + STARTMONITORING;
        StartMonitoringResponse() 
			: InvocationBase(label,getLength(this)) 
		{
		}
      };

	template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case STARTMONITORING: return obj->invoke_startMonitoring(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }
    };

}// protocol
}// mythos
