#include "objects/PerformanceMonitoring.hh"
#include "boot/load_init.hh"
#include "mythos/init.hh"
#include "boot/mlog.hh"
#include "util/events.hh"

namespace mythos {

class PerfMonPlugin
	: protected EventHook<boot::InitLoader&>
{
	public:
		virtual ~PerfMonPlugin(){}

		PerfMonPlugin() {
			event::initLoader.add(this);
		}

		void processEvent(boot::InitLoader& loader) override {
			auto res = loader.csSet(init::PERFORMANCE_MONITORING_MODULE, pm);
    		if (!res) ASSERT(res);

			pm.init();
		}

	protected:
		PerformanceMonitoring pm;
};

PerfMonPlugin perfMonPlugin;

} //mythos
