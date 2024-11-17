#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char *ssid = "xxxxx"; // Enter your Wi-Fi name
const char *password = "xxxxx";  // Enter Wi-Fi password

// MQTT Broker
const char *mqtt_broker = "192.168.1.X"; // Replace X with your laptop's IP address
const char *tof_data_topic = "esp32/tof_data";
const char *wasd_topic = "esp32/wasd";
const char *mqtt_username = ""; // Mosquitto default has no username
const char *mqtt_password = ""; // Mosquitto default has no password
const int mqtt_port = 1883; // Default Mosquitto port

WiFiClient espClient;
PubSubClient client(espClient);

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
    //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    while (!client.connected()) {
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    // Publish and subscribe
    // Create task to publish TOF data
    xTaskCreate(tofTask, "ToF Task", 4096, nullptr, 2, nullptr);
    client.subscribe(wasd_topic);
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
    tofDataLock.lock();
    getTof();
    client.publish(tof_data_topic, tofData);
    tofDataLock.unlock();
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay 500ms
  }
}