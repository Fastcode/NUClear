#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <typeindex>
#include <tuple>
#include "ReactorController.h"
#include "Reactor.h"

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
            on<Trigger<CameraData>, With<MotorData>>([](const CameraData& cameraData, const MotorData& motorData) {
                std::cout << "Double trigger!" << std::endl;
            });

            on<Trigger<CameraData>, With<MotorData>>(std::bind(&Vision::reactInner, this, std::placeholders::_1, std::placeholders::_2));
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
