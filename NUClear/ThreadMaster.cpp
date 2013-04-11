#include "ReactorController.h"
namespace NUClear {
    ReactorController::ThreadMaster::ThreadMaster(ReactorController* parent) :
        ReactorController::BaseMaster(parent) {
    }

    void ReactorController::ThreadMaster::start() {
        for(int i = 0; i < numThreads; ++i) {
            std::unique_ptr<Internal::ThreadWorker> thread;
            m_threads.insert(std::pair<std::thread::id, std::unique_ptr<Internal::ThreadWorker>>(thread->getThreadId(), std::move(thread)));
        }
    }
}
