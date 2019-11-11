#include "boot/load_init.hh"
#include "boot/kernel.hh"
#include "util/events.hh"

namespace mythos {

extern char app_image_start SYMBOL("app_image_start");

/** create the root task EC on the first hardware thread */
class InitLoaderPlugin
   : public EventHook<cpu::ThreadID, bool, size_t>
{
public:
    InitLoaderPlugin() { bootAPEvent.add(this); }

    EventCtrl after(cpu::ThreadID threadID, bool firstBoot, size_t) override {
        if (threadID == 0 && firstBoot) {
            OOPS(boot::InitLoader(&app_image_start).load());
        }
        return EventCtrl::OK;
    }
};

InitLoaderPlugin initloaderplugin;

} // namespace mythos
