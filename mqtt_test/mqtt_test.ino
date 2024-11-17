#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_test.h"

// TwoWire(pinSDA, pinSCL)
const int pinSDA = 19;
const int pinSCL = 18;
TwoWire i2c = TwoWire(0);
std::mutex i2cLock;

// Motor(pinA, pinB, pinEncoder);
const int pinAL = 7;
const int pinBL = 8;
const int pinSleepL = 9;
const int pinAR = 4;
const int pinBR = 5;
const int pinSleepR = 6;

// Motor leftMotor(pinAL, pinBL, 0, pinSleepL);
// Motor rightMotor(pinAR, pinBR, 0, pinSleepR);

// WiFi
const char *ssid = "165KingWest"; // Enter your Wi-Fi name
const char *password = "165Budds";  // Enter Wi-Fi password

// MQTT Broker
const char *mqtt_broker = "192.168.1.166"; // Replace X with your laptop's IP address
const char *tof_data_topic1 = "esp32/tof_data1";
const char *tof_data_topic2 = "esp32/tof_data2";
const char *wasd_topic = "esp32/wasd";
const char *mqtt_username = ""; // Mosquitto default has no username
const char *mqtt_password = ""; // Mosquitto default has no password
const int mqtt_port = 1883; // Default Mosquitto port

WiFiClient espClient;
PubSubClient client(espClient);

bool connectToMqtt() {
    int attempts = 0;
    while (!client.connected() && attempts < 3) {  // Try 3 times
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("Attempt %d: The client %s connects to the MQTT broker\n", attempts + 1, client_id.c_str());
        
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("MQTT broker connected successfully");
            return true;
        } else {
            Serial.print("Failed with state ");
            Serial.println(client.state());
            Serial.println("Retrying in 2 seconds...");
            attempts++;
            delay(2000);
        }
    }
    return false;
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void loop() {
    // client.loop();
    Serial.println("motors should turn on");

    digitalWrite(6, HIGH);
    analogWrite(4, 100);
    analogWrite(5, 0);
    delay(2000);
    analogWrite(4, 0);
    analogWrite(5, 0);
    delay(5000);
}

// Task: Read ToF Sensor Data
void tof1Task(void* param) {
  while (true) {
    getTof();
    tofDataLock.lock();
    // uint8_t randomData[64] = [];
    // String jsonData = "[";
    // for (int i = 0; i < 32; i++) {
    //     if (i > 0) jsonData += ",";
    //     // Reduce range to 0-255 to keep numbers smaller
    //     // randomData[i] = random(0, 255);
    //     jsonData += String(tofData.distance_mm[i]);
    // }
    // jsonData += "]";

    int randomData[64];
    // Use a more compact format without spaces and minimal separators
    String jsonData = "[";
    for (int i = 0; i < 64; i++) {
        if (i > 0) jsonData += ",";
        // Reduce range to 0-255 to keep numbers smaller
        randomData[i] = random(0, 255);
        jsonData += String(randomData[i]);
    }
    jsonData += "]";

    int randomData2[64];
    // Use a more compact format without spaces and minimal separators
    String jsonData2 = "[";
    for (int i = 0; i < 64; i++) {
        if (i > 0) jsonData2 += ",";
        // Reduce range to 0-255 to keep numbers smaller
        randomData2[i] = random(0, 255);
        jsonData2 += String(randomData2[i]);
    }
    jsonData2 += "]";
    
    // Check if client is still connected before publishing
    if (!client.connected()) {
        Serial.println("MQTT client disconnected, attempting to reconnect...");
        if (connectToMqtt()) {
            Serial.println("Reconnected successfully");
        } else {
            Serial.println("Failed to reconnect");
        }
    }
    
    // Attempt to publish and check result
    int publishResult = client.publish(tof_data_topic1, jsonData.c_str());
    int publishResult2 = client.publish(tof_data_topic2, jsonData2.c_str());
    if (publishResult) {
        Serial.println("Published successfully");
        Serial.println(jsonData); // Print the data being sent
    } else {
        Serial.print("Failed to publish. Error code: ");
        Serial.println(publishResult);
    }
    
    tofDataLock.unlock();
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay 500ms
  }
}

void setup1() {
    // Set software serial baud to 115200;
    
    Serial.begin(115200);
    while(!Serial);
    // Set up I2C
    Wire.begin(pinSDA, pinSCL);
    // i2c.setClock(I2C_FREQ);

    // Scan I2C address space
    // while(true){
    Serial.println("Scanning I2C address space...");
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            Serial.println(address, HEX);
        }
    }
    Serial.println("I2C scan complete");
    delay(1000);
    // }
    
    setupMotor1();
    setupMotor2();
    // coast();
}

void setupMotor1() {
    i2c.beginTransmission(0x31);
    i2c.write(0x0C);
    i2c.write(0x03);
    i2c.endTransmission();
    i2c.beginTransmission(0x31);
    i2c.write(0x09);
    i2c.write(0xC2);
    i2c.endTransmission();
    i2c.beginTransmission(0x31);
    i2c.write(0x0D);
    i2c.write(0x18);
    i2c.endTransmission();
    i2c.beginTransmission(0x31);
    i2c.write(0x00);
    i2c.endTransmission();
    i2c.requestFrom(0x31, 1);    // request 1 byte from device with ID 0x20
    while(i2c.available()) {     // device may send less than requested (abnormal)
      char c = i2c.read();       // receive a byte
      Serial.println(c, HEX);     // print the character in hexadecimal
    }

    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    delay(500);
    digitalWrite(9, HIGH);
    // rightMotor.begin();
    // delay(500);
    // rightMotor.enable();
}

void setupMotor2() {
    i2c.beginTransmission(0x34);
    i2c.write(0x0C);
    i2c.write(0x03);
    i2c.endTransmission();
    i2c.beginTransmission(0x34);
    i2c.write(0x09);
    i2c.write(0xC2);
    i2c.endTransmission();
    i2c.beginTransmission(0x34);
    i2c.write(0x0D);
    i2c.write(0x18);
    i2c.endTransmission();
    i2c.beginTransmission(0x34);
    i2c.write(0x00);
    i2c.endTransmission();
    i2c.requestFrom(0x34, 1);    // request 1 byte from device with ID 0x20
    while(i2c.available()) {     // device may send less than requested (abnormal)
      char c = i2c.read();       // receive a byte
      Serial.println(c, HEX);     // print the character in hexadecimal
    }
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    // leftMotor.begin();
    delay(500);
    // leftMotor.enable();
    digitalWrite(6, HIGH);
}

// void tof2Task(void* param) {
//   while (true) {
//     tof2DataLock.lock();
//     getTof();
//     String jsonData = "[";
//     for (int i = 0; i < 64; i++) {
//         if (i > 0) jsonData += ",";
//         // Reduce range to 0-255 to keep numbers smaller
//         jsonData += String(tofData2.distance_mm[i]);
//     }
//     jsonData += "]";
    
//     // Check if client is still connected before publishing
//     if (!client.connected()) {
//         Serial.println("MQTT client disconnected, attempting to reconnect...");
//         if (connectToMqtt()) {
//             Serial.println("Reconnected successfully");
//         } else {
//             Serial.println("Failed to reconnect");
//         }
//     }
    
//     // Attempt to publish and check result
//     int publishResult = client.publish(tof_data_topic2, jsonData.c_str());
//     if (publishResult) {
//         Serial.println("Published successfully");
//         Serial.println(jsonData); // Print the data being sent
//     } else {
//         Serial.print("Failed to publish. Error code: ");
//         Serial.println(publishResult);
//     }
    
//     tof2DataLock.unlock();
//     vTaskDelay(500 / portTICK_PERIOD_MS); // Delay 500ms
//   }
// }

void setup() {
    // Set software serial baud to 115200;
    Serial.begin(115200);
    // Connecting to a WiFi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the Wi-Fi network");
    // Print the IP address once connected
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    connectToMqtt();
    
    // client.subscribe(wasd_topic);

    i2c.begin(pinSDA, pinSCL);

    initToF();

    setupMotor1();
    setupMotor2();
    // coast();

    // Create task to publish TOF data
    TaskHandle_t tofTaskHandle = nullptr;
    xTaskCreate(tof1Task, "ToF Task", 4096, nullptr, 2, &tofTaskHandle);
    if (tofTaskHandle == nullptr) {
        Serial.println("Failed to create ToF task");
    } else {
        Serial.println("ToF task created successfully");
    }
}