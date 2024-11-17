#include "tof.h"

Orientation orientation;
SparkFun_VL53L5CX tof;
VL53L5CX_ResultsData tofData;
std::mutex tofDataLock;

float tofDotProduct[5] = {0};
float tofNormalized[64] = {0};
int tofMatch = -1;

void initToF() {
  if (!tof.begin(0x52 >> 1, i2c)) {
    Serial.println("tough luck. tof not found");
    while (1);
  }

#ifdef DEBUG
  Serial.println("upload complete!");
#endif

  tof.setResolution(64);

  tof.setRangingMode(SF_VL53L5CX_RANGING_MODE::CONTINUOUS);
  // tof.setIntegrationTime(49);
  tof.setRangingFrequency(5);

  tof.startRanging();
}

void readToF() {
  while (1) {
    getTof();
    delay(70);
  }
}

void preprocess(int16_t* m, uint16_t* s, uint8_t* r, uint8_t len) {
  // m==mean s==sigma r==reflectivity
  const uint16_t sigmaThreshold = 30;

  uint8_t bottomRow = orientation==IMU_FACE_UP ? 0 : 7;
  for (uint8_t i = 0; i < len; i++) {
    // m[i] = s[i] > sigmaThreshold ? m[i] + s[i]*2 : m[i];
    // m[i] = m[i] > 1500 ? 1500 : m[i];
    if (i >= bottomRow*8 && i < (bottomRow+1)*8) {
      m[i] = r[i] < 10 ? 200 : m[i];
    }
  }
}

void getTof() {
  bool ready = false;
  do {
    i2cLock.lock();
    ready = tof.isDataReady();
    i2cLock.unlock();
    delay(10);
  } while (!ready);

  i2cLock.lock();
  tofDataLock.lock();
  if (tof.getRangingData(&tofData)) {
    preprocess(tofData.distance_mm, tofData.range_sigma_mm, tofData.reflectance, 64);
  } else {
    Serial.println("tof fetch failed");
  }
  tofDataLock.unlock();
  i2cLock.unlock();
}

template<typename T>
float bufMax(T* buf, uint8_t len) {
  float temp = buf[0];
  for (uint8_t i = 1; i < len; i++) {
    if (buf[i] > temp) temp = buf[i];
  }
  return temp;
}
float bufMax(float* buf, uint8_t len) {
  float temp = buf[0];
  for (uint8_t i = 1; i < len; i++) {
    if (buf[i] > temp) temp = buf[i];
  }
  return temp;
}

template<typename T>
void normalizeBuf(T* src, float* dest, uint8_t len) {
  float max = bufMax<T>(src, len);
  for (uint8_t i = 0; i < len; i++) {
    dest[i] = src[i] / max;
  }
}

template<typename T>
void minBuf(T* src, float* dest, int arg, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    dest[i] = src[i] < arg ? src[i] : arg;
  }
}

template <typename T>
void setZero(T* buf, uint8_t len) {
  for (int i = 0; i < len; i++) {
    buf[i] = 0;
  }
}

int16_t avgDistance(uint8_t row) {
  int16_t distance = 0;
  tofDataLock.lock();
  for (uint8_t i = 0; i < 8; i++) {
    distance += tofData.distance_mm[row*8 + i] / 8;
  }
  tofDataLock.unlock();
  return distance;
}