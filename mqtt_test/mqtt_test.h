#ifndef MAIN_H
#define MAIN_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include <Wire.h>
#include "motor.h"
#include "tof.h"
#include <mutex>

// uncomment to print more verbose messages
// #define DEBUG

using namespace std::chrono_literals;

constexpr uint32_t I2C_FREQ = 400000;
// 1470 for the slowest motors

extern Motor leftMotor;
extern Motor rightMotor;
extern TwoWire i2c;
extern std::mutex i2cLock;
// extern VL53L5CX_ResultsData tofData;
extern std::mutex tofDataLock;

#endif // MAIN_H
