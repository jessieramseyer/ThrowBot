#include "esp32.h"

// Replace with your network credentials
const char* ssid = "REPLACE_IWTH_YOUR_SSID";
const char* password = "REPLACE_IWTH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Motor(pinA, pinB, pinEncoder);
const int pinAL = 7;
const int pinBL = 8;
const int pinSleepL = 9;
const int pinAR = 4;
const int pinBR = 5;
const int pinSleepR = 6;

Motor leftMotor(pinAL, pinBL, 0, pinSleepL);
Motor rightMotor(pinAR, pinBR, 0, pinSleepR);

// TwoWire(pinSDA, pinSCL)
const int pinSDA = 19;
const int pinSCL = 18;
TwoWire i2c = TwoWire(0);
std::mutex i2cLock;

template<typename T>
void printBuf(const T* const buf, uint8_t col, uint8_t row=1) {
  for (int j = 0; j < row; j++) {
    for (int i = 0; i < col; i++) {
      Serial.print(buf[i + j*col]);
      Serial.print(", ");
    }
    Serial.println();
  }
}

template<typename T>
void printBufBytes(const T* const buf, uint8_t len) {
  uint16_t size = len * sizeof(T);
  Serial.write((uint8_t*)buf, size);
  Serial.write("\n");
}

void setup() {
  rightMotor.begin();
  delay(1000);
  rightMotor.enable();
}

void setup1() {

  Serial.begin(115200);
  while(!Serial);
  Serial.println("firmware start!");

  i2c.begin(pinSDA, pinSCL);
  i2c.setClock(I2C_FREQ);

  initToF();

  // Start tasks
  xTaskCreate(tofTask, "ToF Task", 4096, nullptr, 2, nullptr);
  xTaskCreate(encoderTask, "Encoder Task", 4096, nullptr, 2, nullptr);
  xTaskCreate(motorControlTask, "Motor Control Task", 4096, nullptr, 1, nullptr);

  // Initialize LittleFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
        return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html");
  });

  server.on("/tof_data", HTTP_GET, [](AsyncWebServerRequest *request){
      String response = "";
      tofDataLock.lock();
      for (int i = 0; i < 64; i++) {
          response += String(tofData.distance_mm[i]);
          if (i < 63) response += ",";
      }
      tofDataLock.unlock();
      request->send(200, "text/plain", response);
  });

  // Start server
  server.begin();

  leftMotor.begin();
  rightMotor.begin();
  delay(1000);
  rightMotor.enable();
  leftMotor.enable();
  coast();
}

void loop() {
  rightMotor.rotateCW(80);
  delay(3000);
  rightMotor.coast();
  delay(1000);
}

// Task: Read ToF Sensor Data
void tofTask(void* param) {
  while (true) {
    tofDataLock.lock();
    getTof();
    tofDataLock.unlock();
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay 500ms
  }
}

// Task: Read Encoder Data
void encoderTask(void* param) {
  while (true) {
    updateBothEncoders();
    vTaskDelay(50 / portTICK_PERIOD_MS); // Delay 50ms
  }
}

// Task: Motor Control Logic
void motorControlTask(void* param) {
  while (true) {

    tofDataLock.lock();
    uint16_t currentTofData[64];
    memcpy(currentTofData, tofData.distance_mm, 64);
    tofDataLock.unlock();

    if (currentTofData[0] < 300) { // Obstacle detected
      leftMotor.coast();
      rightMotor.coast();
    } else {
      leftMotor.rotateCW(100);
      rightMotor.rotateCW(100);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Delay 100ms
  }
}