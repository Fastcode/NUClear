#include <iostream>
#include <string>
#include "Reactor.h"
#include "ReactorController.h"

struct CameraData {
    int camData;
};

struct MotorData {
    float motorData;
};

class Vision : public NUClear::Reactor {
    public:
        Vision() {
            reactOn<Vision, CameraData, MotorData>();
        }

        void react(const CameraData& cameraData, const MotorData& mData) {
            std::cout << "No way" << std::endl;
            std::cout << "CameraData:" << cameraData.camData << std::endl;
            std::cout << "MotorData:" << mData.motorData << std::endl;
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
}
