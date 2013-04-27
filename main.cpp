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

class RandomData {
    public:
        RandomData(std::string data) : data(data) {};
        std::string data;
};

class Vision : public NUClear::Reactor {
    public:
        Vision(NUClear::ReactorController& control) : Reactor(control) {
            
            // This section of code here is for timing how long our API system is taking
            on<Trigger<std::chrono::time_point<std::chrono::steady_clock>>, With<int, double>>([this](const std::chrono::time_point<std::chrono::steady_clock>& point, const int& i, const double& avg) {
                auto now = std::chrono::steady_clock::now();
                
                if(i > 1000000) {
                    std::cout << "Average API time: " << avg << " ns" << std::endl;
                    reactorController.shutdown();
                }
                else {
                    double soFar = (avg*i);
                    double timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch() - point.time_since_epoch()).count();
                    
                    double total = (soFar + timeTaken)/(i+1);
                    
                    reactorController.emit(new double(total));
                    reactorController.emit(new int(i + 1));
                    reactorController.emit(new std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::now()));
                }
            });
            
            on<Trigger<RandomData>, With<Linked<CameraData>>>([](const RandomData& randomData, const CameraData& cameraData) {
                std::cout << "Got linked data" << std::endl;
            });
        }
};

int main(int argc, char** argv) {
    NUClear::ReactorController controller;
    
    controller.install<Vision>();
    
    controller.emit(new CameraData());
    controller.emit(new RandomData("Hi!"));
    
    std::cout << "Starting speed test" << std::endl << std::endl;
    
    controller.emit(new int(0));
    controller.emit(new double(0));
    controller.emit(new std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::now()));
    
    auto time = std::chrono::steady_clock::now();
    controller.start();
    std::cout << "Total Runtime: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time).count() << " ms" << std::endl;
    
    return 0;
}
