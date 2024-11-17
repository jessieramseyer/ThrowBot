import paho.mqtt.client as mqtt

class MQTTClient:
    # MQTT Configuration
    MQTT_BROKER = "localhost"  # Replace X with your broker IP
    MQTT_PORT = 1883
    TOF1_TOPIC = "esp32/tof_data1"
    TOF2_TOPIC = "esp32/tof_data2"

    def __init__(self):
        self.client = mqtt.Client()
        self.connect()

    def connect(self):
        """Connect to the MQTT broker and start the loop"""
        self.client.connect(self.MQTT_BROKER, self.MQTT_PORT, 60)
        self.client.loop_start()

    def disconnect(self):
        """Disconnect from the MQTT broker"""
        self.client.loop_stop()
        self.client.disconnect()

    def publish(self, topic, message):
        """Publish a message to a topic"""
        self.client.publish(topic, message)

    def subscribe(self, topic, callback):
        """Subscribe to a topic with a callback"""
        self.client.on_message = callback
        self.client.subscribe(topic)

# Create a singleton instance
mqtt_client = MQTTClient()
