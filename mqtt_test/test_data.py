from mqtt_client import mqtt_client
import numpy as np
import json
import time

try:
    while True:
        # Generate random 8x8 arrays for both sensors
        tof1_data = np.random.randint(0, 1000, (64)).tolist()
        tof2_data = np.random.randint(0, 1000, (64)).tolist()
        
        # Publish data using the centralized client
        mqtt_client.publish(mqtt_client.TOF1_TOPIC, json.dumps(tof1_data))
        mqtt_client.publish(mqtt_client.TOF2_TOPIC, json.dumps(tof2_data))
        
        time.sleep(0.5)

except KeyboardInterrupt:
    print("Stopping data generation...")
    mqtt_client.disconnect()
