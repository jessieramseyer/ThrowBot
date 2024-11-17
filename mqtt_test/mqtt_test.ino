#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_test.h"

// TwoWire(pinSDA, pinSCL)
const int pinSDA = 19;
const int pinSCL = 18;
TwoWire i2c = TwoWire(0);
std::mutex i2cLock;

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
    client.loop();
}

// Task: Read ToF Sensor Data
void tofTask(void* param) {
  while (true) {
    // tofDataLock.lock();
    // getTof();
    // Generate random 8x8 array of values between 0-1000
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
    int publishResult2 = client.publish(tof_data_topic2, jsonData.c_str());
    if (publishResult) {
        Serial.println("Published successfully");
        Serial.println(jsonData); // Print the data being sent
    } else {
        Serial.print("Failed to publish. Error code: ");
        Serial.println(publishResult);
    }
    
    // tofDataLock.unlock();
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay 500ms
  }
}

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
    
    client.subscribe(wasd_topic);

    i2c.begin(pinSDA, pinSCL);
    i2c.setClock(I2C_FREQ);

    initToF();

    // Create task to publish TOF data
    TaskHandle_t tofTaskHandle = nullptr;
    xTaskCreate(tofTask, "ToF Task", 4096, nullptr, 2, &tofTaskHandle);
    if (tofTaskHandle == nullptr) {
        Serial.println("Failed to create ToF task");
    } else {
        Serial.println("ToF task created successfully");
    }
}