#include "esp32.h"

// Replace with your network credentials
const char* ssid = "REPLACE_IWTH_YOUR_SSID";
const char* password = "REPLACE_IWTH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Motor(pinA, pinB, pinEncoder);
const int pinAL = 7;
const int pinBL = 8;
const int pinEncL = 9;
const int pinAR = 4;
const int pinBR = 5;
const int pinEncR = 6;

// Motor leftMotor(D35, D6, D5);
// Motor rightMotor(D29, D11, D24);

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

  Serial.begin(115200);
  while(!Serial);
  Serial.println("firmware start!");

  i2c.begin(pinSDA, pinSCL);
  i2c.setClock(I2C_FREQ);

  initToF();

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

  // Start periodic ToF reading
if (xTaskCreate([](void*) {
      while (true) {
          getTof();
          delay(500); // Update data every 500ms
      }
  }, 
  "ToFReader", // Task name
  4096,        // Stack size
  nullptr,     // Task parameters
  1,           // Task priority
  nullptr      // Task handle
) != pdPASS) {
    Serial.println("Failed to create ToF reader task");
}

  // leftMotor.begin();
  // rightMotor.begin();
  // coast();
  // registerEncoderISRs();

  // motorEncTask.start(updateBothEncoders); // polling update
  // motorSpeedTask.start(calculateMotorSpeeds);
  // tofInputTask.start(readToF);
  // debugPrinter.start(printDebugMsgs);

  // straightDrivePID.SetOutputLimits(-255 + pwm_straight_drive, 255 - pwm_straight_drive);
  // straightDrivePID.SetMode(AUTOMATIC);

  // wait for initial measurements to come through
  delay(200);
  // Serial.println("all systems go");

}

void loop() {
  delay(500);
}