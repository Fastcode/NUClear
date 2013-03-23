#include <iostream>
#include <string>
#include "NUClear/Reactor.h"
#include "NUClear/ReactorController.h"
#include "NUClear/Every.h"

struct CameraData {
    int camData;
};

struct MotorData {
    float motorData;
};

void free(const CameraData& cameraData, const MotorData& mData) {
    std::cout << "No way" << std::endl;
    std::cout << "CameraData:" << cameraData.camData << std::endl;
    std::cout << "MotorData:" << mData.motorData << std::endl;
}

class Vision : public NUClear::Reactor {
    public:
        Vision() {
            reactOn<Vision, CameraData, MotorData>();
            reactOn<Vision, MotorData>();
            reactOn<Vision, NUClear::Every<1, std::chrono::seconds>>();
            //on<CameraData, MotorData>()
            //.reactWith(std::bind(&Vision::react, this));
            //on<CameraData, MotorData>().reactWith(std::bind(&Vision::react, this));
            /*.reactWith([](const CameraData& cameraData, const MotorData& mData) {
                std::cout << "woo" << std::endl;
            });*/
        }

        void react(const CameraData& cameraData, const MotorData& mData) {
            std::cout << "No way" << std::endl;
            std::cout << "CameraData:" << cameraData.camData << std::endl;
            std::cout << "MotorData:" << mData.motorData << std::endl;
        }
    
        void react(const MotorData& motorData) {
            std::cout << "DAT MOTOR" << std::endl;
        }
    
        void react(const NUClear::Every<1, std::chrono::seconds>& t) {
            std::cout << "ONE PER SECOND" << std::endl;
        }
};



int main(int argc, char** argv) {
    Vision vision;

    CameraData* cData = new CameraData();
    cData->camData = 5;

    MotorData* mData = new MotorData();
    mData->motorData = 10;

    NUClear::ReactorControl.emit<MotorData>(mData);
    NUClear::ReactorControl.emit<CameraData>(cData);

    NUClear::ReactorControl.stop();
    std::cerr << "End of main" << std::endl;
}
