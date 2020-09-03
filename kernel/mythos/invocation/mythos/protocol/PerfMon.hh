#pragma once

#include "mythos/protocol/common.hh"

namespace mythos {
namespace protocol {

	struct PerformanceMonitoring {
		constexpr static uint8_t proto = mythos::protocol::PERFMON;

		enum Methods : uint8_t {
			INITIALIZECOUNTERS,
			COLLECTVALUES,
			MEASURECOLLECTCALLDURATION,
			MEASURECOLLECTLATENCY
		};

		struct InitializeCounters : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + INITIALIZECOUNTERS;
			InitializeCounters() 
			: InvocationBase(label,getLength(this)) 
			{
			}
 		};
		struct InitializeCountersResponse : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + INITIALIZECOUNTERS;
			InitializeCountersResponse() 
			: InvocationBase(label,getLength(this)) 
			{
			}
		};

		struct CollectValues : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + COLLECTVALUES;
			CollectValues() 
			: InvocationBase(label,getLength(this)) 
			{
			}
 		};
		struct CollectValuesResponse : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + COLLECTVALUES;
			CollectValuesResponse() 
			: InvocationBase(label,getLength(this)) 
			{
			}
		};

		struct MeasureCollectCallDuration : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + MEASURECOLLECTCALLDURATION;
			MeasureCollectCallDuration() 
			: InvocationBase(label,getLength(this)) 
			{
			}
 		};
		struct MeasureCollectCallDurationResponse : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + MEASURECOLLECTCALLDURATION;
			MeasureCollectCallDurationResponse() 
			: InvocationBase(label,getLength(this)) 
			{
			}
		};

		struct MeasureCollectLatency : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + MEASURECOLLECTLATENCY;
			MeasureCollectLatency() 
			: InvocationBase(label,getLength(this)) 
			{
			}
 		};
		struct MeasureCollectLatencyResponse : public InvocationBase {
			constexpr static uint16_t label = (proto<<8) + MEASURECOLLECTLATENCY;
			MeasureCollectLatencyResponse() 
			: InvocationBase(label,getLength(this)) 
			{
			}
		};

		template<class IMPL, class... ARGS>
		static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
			switch(Methods(m)) {
				case INITIALIZECOUNTERS: return obj->invoke_initializeCounters(args...);
				case COLLECTVALUES: return obj->invoke_collectValues(args...);
				case MEASURECOLLECTCALLDURATION: return obj->invoke_measureCollectCallDuration(args...);
				case MEASURECOLLECTLATENCY: return obj->invoke_measureCollectLatency(args...);
				default: return Error::NOT_IMPLEMENTED;
			}
		}
	};

}// protocol
}// mythos
