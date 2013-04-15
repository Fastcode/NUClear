/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
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
            on<Trigger<MotorData>, With<CameraData>>([](const MotorData& cameraData, const CameraData& motorData) {
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

    std::chrono::nanoseconds a(std::chrono::steady_clock::now().time_since_epoch());
    controller.emit(new MotorData());
    std::chrono::nanoseconds b(std::chrono::steady_clock::now().time_since_epoch());
    controller.emit(new CameraData());
    std::chrono::nanoseconds c(std::chrono::steady_clock::now().time_since_epoch());
    controller.emit(new MotorData());
    std::chrono::nanoseconds d(std::chrono::steady_clock::now().time_since_epoch());
    controller.emit(new CameraData());
    std::chrono::nanoseconds e(std::chrono::steady_clock::now().time_since_epoch());
    controller.emit(new MotorData());
    std::chrono::nanoseconds f(std::chrono::steady_clock::now().time_since_epoch());
    controller.emit(new CameraData());
    std::chrono::nanoseconds g(std::chrono::steady_clock::now().time_since_epoch());
    
    std::cout << "Emit MotorData 1 Elapsed: " << (b - a).count() << std::endl;
    
    std::cout << "Emit CameraData 1 Elapsed: " << (c - b).count() << std::endl;
    
    std::cout << "Emit MotorData 2 Elapsed: " << (d - c).count() << std::endl;
    
    std::cout << "Emit CameraData 2 Elapsed: " << (e - d).count() << std::endl;
    
    std::cout << "Emit MotorData 3 Elapsed: " << (f - e).count() << std::endl;
    
    std::cout << "Emit CameraData 3 Elapsed: " << (g - f).count() << std::endl;
}
