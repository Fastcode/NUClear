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
            /*
            on<Trigger<MotorData>, With<CameraData>>([](const MotorData& cameraData, const CameraData& motorData) {
                std::cout << "Double trigger!" << std::endl;
            });

            on<Trigger<CameraData>, With<MotorData>>(std::bind(&Vision::reactInner, this, std::placeholders::_1, std::placeholders::_2));
            
            on<Trigger<Every<1, std::chrono::seconds>>>([](const std::chrono::time_point<std::chrono::steady_clock>& time) {
                std::cout << "Every 1 seconds called" << std::endl;
            });
            
            on<Trigger<Every<2, std::chrono::seconds>>>([](const std::chrono::time_point<std::chrono::steady_clock>& time) {
                std::cout << "Every 2 seconds called" << std::endl;
            });
            
            on<Trigger<CameraData>, With<>, Options<Sync<Vision>, Single, Priority<NUClear::REALTIME>>>([](const CameraData& cameraData){
                std::cout << "With Options" << std::endl;
            });
            
            on<Trigger<Last<3, RandomData>>>([](const std::vector<std::shared_ptr<const RandomData>>& data) {
                std::cout << "I'm getting the last 3 CameraDatas!: " << data.size() << std::endl;
                
                for(auto it = std::begin(data); it != std::end(data); ++it) {
                    if(*it) {
                        std::cout << (*it)->data << std::endl;
                    }
                    else {
                        std::cout << "nullptr" << std::endl;
                    }
                }
            });
            
            on<Trigger<Last<5, RandomData>>>([](std::vector<std::shared_ptr<const RandomData>>& data) {
                std::cout << "I'm getting the last 5 CameraDatas!: " << data.size() << std::endl;
                
                for(auto it = std::begin(data); it != std::end(data); ++it) {
                    if(*it) {
                        std::cout << (*it)->data << std::endl;
                    }
                    else {
                        std::cout << "nullptr" << std::endl;
                    }
                }
            });
            */
            
            on<Trigger<Every<1, std::chrono::hours>>>([](const std::chrono::time_point<std::chrono::steady_clock>& time){});
            
            // This section of code here is for timing how long our API system is taking
            on<Trigger<std::chrono::time_point<std::chrono::steady_clock>>, With<int, long>>([this](const std::chrono::time_point<std::chrono::steady_clock>& point, const int& i, const long& avg) {
                auto now = std::chrono::steady_clock::now();
                
                if(i > 100000) {
                    std::cout << "Average API time: " << avg << " ns" << std::endl;
                }
                else {
                    reactorController.emit(new long(static_cast<long>(((avg * i) + std::chrono::duration_cast<std::chrono::nanoseconds>((now.time_since_epoch() - point.time_since_epoch())).count())/(i+1))));
                    reactorController.emit(new int(i + 1));
                    reactorController.emit(new std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::now()));
                }
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
    
    
    controller.emit(new RandomData("Data 1"));
    controller.emit(new RandomData("Data 2"));
    controller.emit(new RandomData("Data 3"));
    controller.emit(new RandomData("Data 4"));
    controller.emit(new RandomData("Data 5"));
    controller.emit(new RandomData("Data 6"));

    std::chrono::time_point<std::chrono::steady_clock> a(std::chrono::steady_clock::now());
    controller.emit(new MotorData());
    std::chrono::time_point<std::chrono::steady_clock> b(std::chrono::steady_clock::now());
    controller.emit(new CameraData());
    std::chrono::time_point<std::chrono::steady_clock> c(std::chrono::steady_clock::now());
    controller.emit(new MotorData());
    std::chrono::time_point<std::chrono::steady_clock> d(std::chrono::steady_clock::now());
    controller.emit(new CameraData());
    std::chrono::time_point<std::chrono::steady_clock> e(std::chrono::steady_clock::now());
    controller.emit(new MotorData());
    std::chrono::time_point<std::chrono::steady_clock> f(std::chrono::steady_clock::now());
    controller.emit(new CameraData());
    std::chrono::time_point<std::chrono::steady_clock> g(std::chrono::steady_clock::now());
    
    std::cout << "Emit MotorData 1 Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count() << std::endl;
    
    std::cout << "Emit CameraData 1 Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(c - b).count() << std::endl;
    
    std::cout << "Emit MotorData 2 Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(d - c).count() << std::endl;
    
    std::cout << "Emit CameraData 2 Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(e - d).count() << std::endl;
    
    std::cout << "Emit MotorData 3 Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(f - e).count() << std::endl;
    
    std::cout << "Emit CameraData 3 Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(g - f).count() << std::endl;
    
    std::cout << "Starting API Speed Test" << std::endl;
    
    controller.emit(new int(0));
    controller.emit(new long(0));
    controller.emit(new std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::now()));
    
    controller.start();
    
    return 0;
}
