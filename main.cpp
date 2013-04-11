#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <typeindex>
#include <tuple>
#include <chrono>
#include "NUClear/NUClear.h"

class CameraData {
    public: 
        CameraData() : data("Class::CameraData") {}
        std::string data;
};

class MotorData {
    public: 
        MotorData() : data("Class::MotorData") {}
        std::string data;
};

class Vision : public NUClear::Reactor {
    public:
        Vision(NUClear::ReactorController& control) : Reactor(control) {
            //on<Trigger<MotorData>>();
           /* on<Trigger<MotorData>, With<CameraData>>([](const MotorData& cameraData, const CameraData& motorData) {
                std::cout << "Double trigger!" << std::endl;
            });

            on<Trigger<CameraData>, With<MotorData>>(std::bind(&Vision::reactInner, this, std::placeholders::_1, std::placeholders::_2));
            
            on<Trigger<Every<1, std::chrono::seconds>>>([](const Every<1, std::chrono::seconds>& time) {
                std::cout << "Every 1 seconds called" << std::endl;
            });
            
            on<Trigger<Every<10, std::chrono::milliseconds>>>([](const Every<10, std::chrono::milliseconds>& time) {
                std::cout << "Every 100 milliseconds called" << std::endl;
            });
            
            on<Trigger<Every<1000>>>([](const Every<1000>& time) {
                std::cout << "Every 1000 milliseconds called" << std::endl;
            });
            */
            on<Trigger<CameraData>, With<>, Options<Sync<Vision>, Single, Priority<NUClear::REALTIME>>>([](const CameraData& cameraData){
                std::cout << "With Options" << std::endl;
            });
        }
    private:
        void reactInner(const CameraData& cameraData, const MotorData& motorData) {
            std::cout << "ReactInner on cameraData, got: [" << cameraData.data << "], [" << motorData.data << "]" << std::endl;
        }
};

int main(int argc, char** argv) {
    NUClear::ReactorController controller;
    
    controller.install<Vision>();

    controller.emit(new MotorData());
    controller.emit(new CameraData());
}
