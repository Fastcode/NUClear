#include <iostream>
#include <functional>
#include <string>
#include <typeindex>
#include <map>
#include <vector>


struct CameraData {
    int camData;
};

struct MotorData {
    float motorData;
};

class ReactorCore {
    public:
        template <typename T>
        void emit(T* message) {
            m_cache[typeid(T)] = message;    
        }

        template <typename T>
        T& get() {
            return *(static_cast<T*>(m_cache[typeid(T)]));
        }

        std::map<std::type_index, void*> m_cache;
};

ReactorCore core;

template <typename ChildType>
class Reactor {
    public:
        template <typename T>
        void notify() {
            auto callbacks = getCallbacks<T>();
            std::cout << "Doing notify" << std::endl;
            for(auto callback = callbacks.begin(); callback != callbacks.end(); ++callback) {
                std::cout << "Callback" << std::endl;
                (*callback)();
            }
        }

        template <typename TTrigger, typename... TWith>
        void on(void callback(const TTrigger&, const TWith&...)) {
            std::vector<std::function<void ()>>& reactors = getCallbacks<TTrigger>();
            std::cout << "Doing on" << std::endl;
            reactors.push_back([callback]() {
                callback(core.get<TTrigger>(), core.get<TWith>()...);
            });
        }

        template <typename TTrigger, typename... TWith>
        void on() {
            std::vector<std::function<void ()>>& reactors = getCallbacks<TTrigger>();
            std::cout << "Doing auto-on" << std::endl;
            reactors.push_back([this]() {
                static_cast<ChildType*>(this)->react(core.get<TTrigger>(), core.get<TWith>()...);
            });
        }

        std::map<std::type_index, std::vector<std::function<void ()>>> m_reactors;


        template <typename T>
        std::vector<std::function<void ()>>& getCallbacks() {
            if(m_reactors.find(typeid(T)) == m_reactors.end()) {
                m_reactors[typeid(T)] = std::vector<std::function<void ()>>();
            }

            return m_reactors[typeid(T)];
        }
};

class Vision : public Reactor<Vision> {
    public:
        Vision() {
            //on<CameraData, MotorData>(&reactTest);
            //on<CameraData, MotorData>(std::bind(&Vision::react, this));
            on<CameraData, MotorData>();
            //on<const CameraData&, const MotorData&>(std::bind(&Vision::react, this));
            //visionReactorController.on<CameraData, MotorData>(std::bind(Vision::react, this));        
        }

        void react(const CameraData& cameraData, const MotorData& mData) {
            std::cout << "No way" << std::endl;
            std::cout << "CameraData:" << cameraData.camData << std::endl;
            std::cout << "MotorData:" << mData.motorData << std::endl;
        }
};

int main(int argc, char** argv) {
    Vision vision;

    CameraData cData;
    cData.camData = 5;

    MotorData mData;
    mData.motorData = 10;

    core.emit<CameraData>(&cData);
    core.emit<MotorData>(&mData);
    vision.notify<CameraData>();
    vision.notify<MotorData>();
}
