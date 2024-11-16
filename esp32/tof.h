#ifndef TOF_H
#define TOF_H
#include <mutex>
#include <SparkFun_VL53L5CX_Library.h>

typedef enum {
  IMU_FACE_UP,
  IMU_FACE_DOWN,
  UNKNOWN,
} Orientation;

extern Orientation orientation;

constexpr uint8_t kernelRows = 8;
constexpr uint8_t kernelCols = 5;
constexpr uint8_t strideLen = kernelRows - kernelCols + 1;

extern float tofDotProduct[5];
extern float tofNormalized[64];
extern int tofMatch;

extern SparkFun_VL53L5CX tof;
extern VL53L5CX_ResultsData tofData;
extern std::mutex tofDataLock;
extern std::mutex i2cLock;
extern TwoWire i2c;

void initToF();
void readToF();
void getTof();

#endif // TOF_H
